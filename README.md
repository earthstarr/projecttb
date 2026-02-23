# ProjectTB — Turn-Based RPG (Unreal Engine 5)

> UE 5.6 + GAS(Gameplay Ability System) 기반 턴제 RPG 전투 시스템 구현 프로젝트

---

## 개요

3인 파티 vs 적의 턴제 전투를 구현한 프로젝트입니다.
언리얼 엔진의 GAS를 활용해 어빌리티·스탯·데미지 처리를 구조화하고,
Speed 기반 라운드 순서와 키보드 UI 조작까지 C++로 설계했습니다.

---

## 주요 기술

| 항목 | 내용 |
|------|------|
| Engine | Unreal Engine 5.6.1 |
| Language | C++ 위주 (Blueprint는 데이터/비주얼 담당) |
| Ability System | GAS (GameplayAbilities, GameplayTags, GameplayEffects) |
| Input | Enhanced Input System |
| UI | UMG (C++ 기반 클래스 + Blueprint 비주얼) |

---

## 구현 내용

### 전투 시스템
- Speed 어트리뷰트 기반 라운드 순서 자동 정렬
- 상태머신(EBattlePhase)으로 전투 흐름 관리 (`BattleStart → PlayerTurn → ExecutingAction → ...`)
- 사망 유닛 즉시 제거 및 라운드 재구성

### GAS 연동
- `ABattleCombatant`에 ASC 직접 보유 (PlayerState 아님)
- Custom Execution Calculation(`TBDamageExecution`)으로 데미지 공식 구현
  - 물리/마법 데미지 분리, 방어력 감소 공식, 분산(±10%), 크리티컬
- `IncomingDamage` 메타 어트리뷰트로 데미지 파이프라인 설계
- GameplayEffect로 비용(MP/Stamina) 및 초기 스탯 적용

### 어빌리티
- `UTBGameplayAbility` C++ 기반 클래스 + Blueprint 서브클래스 구조
- Melee(캐릭터 이동 후 공격) / Ranged(제자리 이펙트) 타입 분리
- AnimNotify(`OnHit` / `OnCast`) 타이밍으로 GE 적용

### UI
- 키보드 전용 배틀 메뉴 (↑↓ 이동 / Enter 확인 / Esc 취소)
- 좌측 턴 순서 패널 (`GetUpcomingTurns(N)`)
- HP / MP / Stamina 하단 상태 바
- 플로팅 데미지 텍스트 (크리티컬 시 색상 강조)

### 캐릭터
| 직업 | 특징 |
|------|------|
| 검사 (Swordsman) | 고 HP/방어, 근접 탱커 |
| 마법사 (Mage) | 고 마법 공격, 광역 딜러 |
| 궁수 (Archer) | 고 Speed, 선공 특화 원거리 딜러 |

---

## 아키텍처

```
BattleManager (턴·상태머신)
  ├─ ABattleCombatant (ASC, AttributeSet, 어빌리티)
  │    ├─ ABattlePlayerCharacter
  │    └─ ABattleEnemyCharacter
  ├─ TBBattleHUD
  │    ├─ BattleMenuWidget  (입력 처리)
  │    ├─ TurnOrderWidget   (순서 표시)
  │    └─ CharacterStatusWidget (HP/MP/Stamina)
  └─ TBGameInstance (레벨 간 데이터, GAS 전역 초기화)
```

---

## 데미지 공식

```
Reduction   = Defense / (Defense + 100)
BaseDamage  = Attack * AbilityMultiplier
FinalDamage = BaseDamage * (1 - Reduction) * Variance(0.9 ~ 1.1)
크리티컬 발생 시: FinalDamage * CriticalMultiplier (기본 1.5x)
```

---

## 실행 환경

- Unreal Engine 5.6.1
- Visual Studio 2022
- Windows 10 / 11
