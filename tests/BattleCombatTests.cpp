#include "../src/BattleCombat.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using namespace BattleCombat;

int failures = 0;

void Expect(bool condition, std::string_view name) {
	if (condition) return;
	std::cerr << "FAILED: " << name << '\n';
	++failures;
}

bool HasNoEvent(AttackEvents events) {
	return !events.shieldHit && !events.hit && (events.damage == 0);
}

void TestCombatResult() {
	CombatValues player{ 14, 9 };
	CombatValues enemy{ 20, 3 };
	const auto result = ResolveCombat(player, enemy, 6, 4, false);
	Expect((result.damageToEnemy == 4) && (result.damageToPlayer == 6),
		"both damage totals stop at the target's current HP");
	player.attack = 99;
	enemy.defense = 99;
	Expect((result.player.attack == 14) && (result.player.defense == 9)
		&& (result.enemy.attack == 20) && (result.enemy.defense == 3),
		"the result preserves the original combat values independently of later previews");

	const auto blocked = ResolveCombat({ 14, 9 }, { 20, 3 }, 60, 40, true);
	Expect((blocked.damageToEnemy == 0) && (blocked.damageToPlayer == 11)
		&& (blocked.player.attack == 14),
		"damage cancellation blocks only the player's damage and preserves raw attack");
	const auto defended = ResolveCombat({ 3, 8 }, { 5, 7 }, 60, 40, false);
	Expect((defended.damageToEnemy == 0) && (defended.damageToPlayer == 0),
		"defense cannot turn damage negative");
	const auto depleted = ResolveCombat({ 14, 0 }, { 20, 0 }, -3, 0, false);
	Expect((depleted.damageToEnemy == 0) && (depleted.damageToPlayer == 0),
		"nonpositive HP cannot produce a negative capped damage total");
	const auto extremes = ResolveCombat(
		{ std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::min() },
		{ std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::min() },
		7, 11, false);
	Expect((extremes.damageToEnemy == 11) && (extremes.damageToPlayer == 7),
		"large raw differences are capped before narrowing to a damage total");
}

void TestContactPhaseSequence() {
	AttackAnimation animation;
	animation.Start(14, 9, 5, 100, 0.4);
	Expect((animation.phase == AttackPhase::Delay) && (animation.elapsed == 0)
		&& (animation.attack == 14) && (animation.defense == 9),
		"a delayed attack begins with its original display values");
	Expect(HasNoEvent(animation.Update(0.39)) && (animation.phase == AttackPhase::Delay),
		"the opening delay holds before the arc");
	Expect(HasNoEvent(animation.Update(0.02)) && (animation.phase == AttackPhase::Arc)
		&& (animation.elapsed == 0),
		"the delay enters the arc and discards excess frame time");
	Expect(HasNoEvent(animation.Update(0.49)) && (animation.phase == AttackPhase::Arc),
		"the arc continues until its travel duration");
	const auto contact = animation.Update(0.01);
	Expect(contact.shieldHit && !contact.hit && (contact.damage == 0)
		&& (animation.phase == AttackPhase::Defense) && (animation.elapsed == 0),
		"entering defense emits one shield contact and starts a fresh clock");
	Expect(HasNoEvent(animation.Update(0.25)) && (animation.attack == 11)
		&& (animation.defense == 6),
		"defense interpolates and truncates both display values");
	Expect(HasNoEvent(animation.Update(0.25)) && (animation.attack == 9)
		&& (animation.defense == 4),
		"remaining in defense never repeats its shield event");
	Expect(HasNoEvent(animation.Update(0.5)) && (animation.phase == AttackPhase::Impact)
		&& (animation.attack == 5) && (animation.defense == 0) && (animation.elapsed == 0),
		"defense finishes with the resolved values before body impact");
	Expect(HasNoEvent(animation.Update(0.5)) && (animation.phase == AttackPhase::Hits)
		&& (animation.elapsed == 0),
		"body impact enters hits without emitting a hit in the same update");
	const auto hit = animation.Update(0);
	Expect(hit.hit && !hit.shieldHit && (hit.damage == 5)
		&& (animation.phase == AttackPhase::Complete) && (animation.elapsed == 0),
		"the first hit is due immediately and the final hit completes the attack");
	Expect(HasNoEvent(animation.Update(10)) && (animation.elapsed == 0),
		"a completed attack cannot emit more events or advance its clock");
}

void TestHitsPreserveDamageAndCadence() {
	AttackAnimation animation;
	animation.Start(17, 0, 17, 100);
	Expect(animation.phase == AttackPhase::Arc,
		"the default attack starts at its arc without an opening delay");
	Expect(HasNoEvent(animation.Update(100)) && (animation.phase == AttackPhase::Impact)
		&& (animation.elapsed == 0) && (animation.attack == 17),
		"zero defense skips contact while a long frame advances only one phase");
	Expect(HasNoEvent(animation.Update(0.49)) && (animation.phase == AttackPhase::Impact),
		"the body impact keeps its full duration after a long arc frame");
	Expect(HasNoEvent(animation.Update(0.01)) && (animation.phase == AttackPhase::Hits),
		"body travel finishes before damage begins");

	std::vector<std::int32_t> hits;
	const auto first = animation.Update(0);
	Expect(first.hit && !first.shieldHit && (animation.phase == AttackPhase::Hits),
		"the first split hit leaves later hits pending");
	if (first.hit) hits.push_back(first.damage);
	Expect(HasNoEvent(animation.Update(0.14)),
		"the second hit waits for its scheduled interval");
	const auto second = animation.Update(0.01);
	Expect(second.hit && !second.shieldHit && (animation.phase == AttackPhase::Hits),
		"the second hit arrives at one interval");
	if (second.hit) hits.push_back(second.damage);
	Expect(HasNoEvent(animation.Update(0)),
		"a pending final hit cannot repeat the previous hit's time slot");
	const auto third = animation.Update(0.15);
	Expect(third.hit && !third.shieldHit && (animation.phase == AttackPhase::Complete),
		"the third hit arrives at two intervals and completes the attack");
	if (third.hit) hits.push_back(third.damage);
	Expect(hits == std::vector<std::int32_t>{ 6, 6, 5 },
		"split hits retain their order and sum to the resolved damage of seventeen");
}

void TestLongFramesEmitOneHitPerUpdate() {
	AttackAnimation animation;
	animation.Start(17, 0, 17, 100);
	animation.Update(1);
	animation.Update(1);
	const auto first = animation.Update(100);
	Expect(first.hit && (first.damage == 6) && (animation.phase == AttackPhase::Hits),
		"a long hits frame emits only the first outstanding hit");
	const auto second = animation.Update(0);
	Expect(second.hit && (second.damage == 6) && (animation.phase == AttackPhase::Hits),
		"a later update emits only the next overdue hit");
	const auto third = animation.Update(0);
	Expect(third.hit && (third.damage == 5) && (animation.phase == AttackPhase::Complete),
		"overdue hits finish in order without losing damage");
}

void TestZeroAttackAndBlockedDamage() {
	AttackAnimation animation;
	animation.Start(0, 10, 0, 100);
	Expect(HasNoEvent(animation.Update(0.5)) && (animation.phase == AttackPhase::Hits)
		&& (animation.attack == 0) && (animation.defense == 10),
		"zero attack skips defense and body impact without consuming defense");
	Expect(HasNoEvent(animation.Update(0)) && (animation.phase == AttackPhase::Complete),
		"zero attack completes without phantom damage or sound");

	animation.Start(12, 0, 0, 100);
	Expect(HasNoEvent(animation.Update(0.5)) && (animation.phase == AttackPhase::Hits)
		&& (animation.attack == 0),
		"cancelled damage with no defense skips body impact");
	Expect(HasNoEvent(animation.Update(5)) && (animation.phase == AttackPhase::Complete),
		"cancelled damage never emits hit events");

	animation.Start(4, 10, 0, 100);
	Expect(animation.Update(0.5).shieldHit && (animation.phase == AttackPhase::Defense),
		"a fully absorbed attack still makes shield contact");
	Expect(HasNoEvent(animation.Update(0.2)) && (animation.attack == 2)
		&& (animation.defense == 8),
		"partial defense consumption scales the exchange duration");
	Expect(HasNoEvent(animation.Update(0.2)) && (animation.phase == AttackPhase::Hits)
		&& (animation.attack == 0) && (animation.defense == 6),
		"fully absorbed attack preserves unused defense and skips body impact");
	Expect(HasNoEvent(animation.Update(0)) && (animation.phase == AttackPhase::Complete),
		"a fully absorbed attack completes without hit events");
}

void TestFreshStartResetsProgress() {
	AttackAnimation animation;
	animation.Start(17, 0, 17, 100);
	animation.Update(1);
	animation.Update(1);
	animation.Update(0);
	animation.Update(0.1);
	animation.Start(14, 9, 5, 100, 0.4);
	Expect((animation.phase == AttackPhase::Delay) && (animation.elapsed == 0)
		&& (animation.attack == 14) && (animation.defense == 9),
		"a fresh start resets the phase, clock, and original combat values");
	animation.Update(0.4);
	Expect(animation.Update(0.5).shieldHit,
		"a fresh attack emits its own shield contact");
	animation.Update(1);
	animation.Update(0.5);
	const auto hit = animation.Update(0);
	Expect(hit.hit && (hit.damage == 5) && (animation.phase == AttackPhase::Complete),
		"a fresh attack replaces old split hits and starts at its first hit");
}

} // namespace

int main() {
	TestCombatResult();
	TestContactPhaseSequence();
	TestHitsPreserveDamageAndCadence();
	TestLongFramesEmitOneHitPerUpdate();
	TestZeroAttackAndBlockedDamage();
	TestFreshStartResetsProgress();
	if (failures != 0) return EXIT_FAILURE;
	std::cout << "Battle combat tests passed\n";
	return EXIT_SUCCESS;
}
