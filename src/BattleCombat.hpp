#ifndef BattleCombat_HPP
#define BattleCombat_HPP

#include "BattleDamageRules.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace BattleCombat {

inline constexpr double ArcDuration = 0.5;
inline constexpr double ImpactDuration = 0.5;
inline constexpr double HitInterval = 0.15;

struct CombatValues {
	std::int32_t attack = 0;
	std::int32_t defense = 0;
};

// 攻撃を始める前の値を残しておく．表示の数字が減っても，ダメージは変えない．
struct CombatResult {
	CombatValues player;
	CombatValues enemy;
	std::int32_t damageToEnemy = 0;
	std::int32_t damageToPlayer = 0;
};

inline CombatResult ResolveCombat(CombatValues player, CombatValues enemy,
	std::int32_t playerHp, std::int32_t enemyHp, bool blocksPlayerDamage) {
	std::int64_t playerDamage = player.attack;
	std::int64_t enemyDamage = enemy.attack;
	playerDamage -= enemy.defense;
	enemyDamage -= player.defense;
	playerDamage = std::clamp<std::int64_t>(playerDamage, 0, std::max(0, enemyHp));
	enemyDamage = std::clamp<std::int64_t>(enemyDamage, 0, std::max(0, playerHp));
	if (blocksPlayerDamage) playerDamage = 0;
	return {
		player,
		enemy,
		static_cast<std::int32_t>(playerDamage),
		static_cast<std::int32_t>(enemyDamage),
	};
}

enum class AttackPhase { Delay, Arc, Defense, Impact, Hits, Complete };

// この更新で起きたことだけを返す．音を鳴らす処理はシーン側．
struct AttackEvents {
	bool shieldHit = false;
	bool hit = false;
	std::int32_t damage = 0;
};

class AttackAnimation {
public:
	AttackPhase phase = AttackPhase::Complete;
	double elapsed = 0;
	// こちらは演出中に表示する数字．防御を削る間は少しずつ変わる．
	std::int32_t attack = 0;
	std::int32_t defense = 0;

	void Start(std::int32_t initialAttack, std::int32_t initialDefense, std::int32_t damage,
		std::int32_t maximumHp, double delay = 0) {
		attack = initialAttack;
		defense = initialDefense;
		m_originalAttack = initialAttack;
		m_originalDefense = initialDefense;
		m_delay = delay;
		m_exchange = BattleDamageRules::ResolveDefenseExchange(attack, defense, damage);
		m_defenseDuration = BattleDamageRules::ResolveDefenseExchangeDuration(attack, defense);
		m_hits = BattleDamageRules::SplitDamage(damage, maximumHp);
		m_nextHit = 0;
		ChangePhase((0 < delay) ? AttackPhase::Delay : AttackPhase::Arc);
	}

	AttackEvents Update(double deltaSeconds) {
		AttackEvents events;
		if (phase == AttackPhase::Complete) return events;
		// 段階を切り替えたら時間を0に戻す．一度の更新で先へ飛ばしすぎないように．
		elapsed += deltaSeconds;
		switch (phase) {
		case AttackPhase::Delay:
			if (m_delay <= elapsed) ChangePhase(AttackPhase::Arc);
			break;
		case AttackPhase::Arc:
			if (ArcDuration <= elapsed) {
				if (m_exchange.has_contact) {
					ChangePhase(AttackPhase::Defense);
					events.shieldHit = true;
				} else {
					FinishDefense();
				}
			}
			break;
		case AttackPhase::Defense:
			if (m_defenseDuration <= elapsed) {
				FinishDefense();
			} else {
				double progress = elapsed / m_defenseDuration;
				attack = static_cast<std::int32_t>(m_originalAttack
					+ (static_cast<double>(m_exchange.attack_after) - m_originalAttack) * progress);
				defense = static_cast<std::int32_t>(m_originalDefense
					+ (static_cast<double>(m_exchange.defense_after) - m_originalDefense) * progress);
			}
			break;
		case AttackPhase::Impact:
			if (ImpactDuration <= elapsed) ChangePhase(AttackPhase::Hits);
			break;
		case AttackPhase::Hits:
			if (m_nextHit < m_hits.size()) {
				if (m_nextHit * HitInterval <= elapsed) {
					events.hit = true;
					events.damage = m_hits[m_nextHit];
					++m_nextHit;
				}
			}
			// 最後のダメージを返す更新で Complete にもなるので，呼び出し側は先にヒットを扱う．
			if (m_nextHit == m_hits.size()) ChangePhase(AttackPhase::Complete);
			break;
		case AttackPhase::Complete:
			break;
		}
		return events;
	}

private:
	std::int32_t m_originalAttack = 0;
	std::int32_t m_originalDefense = 0;
	double m_delay = 0;
	double m_defenseDuration = 0;
	BattleDamageRules::DefenseExchange m_exchange;
	std::vector<std::int32_t> m_hits;
	std::size_t m_nextHit = 0;

	void ChangePhase(AttackPhase next) {
		phase = next;
		elapsed = 0;
	}

	void FinishDefense() {
		attack = m_exchange.attack_after;
		defense = m_exchange.defense_after;
		ChangePhase((attack == 0) ? AttackPhase::Hits : AttackPhase::Impact);
	}
};

} // namespace BattleCombat

#endif
