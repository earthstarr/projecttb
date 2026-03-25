# ProjectTB — Turn-Based RPG (Unreal Engine 5)

> UE 5.6 + GAS(Gameplay Ability System) 기반 턴제 RPG 로그라이크 프로젝트

---

## 개요

3인 파티로 던전을 탐험하며 적과 턴제 전투를 펼치는 로그라이크 RPG입니다.
언리얼 엔진의 GAS를 활용해 어빌리티·스탯·데미지 처리를 구조화하고,
주사위 시스템으로 전략적 깊이를 더했으며, 레벨 스트리밍 기반 포탈 시스템으로 매 플레이마다 다른 던전을 생성합니다.
키보드 전용 조작으로 빠른 플레이가 가능하도록 설계했습니다.

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

### 월드 탐험
- 레벨 스트리밍 기반 던전 생성 (포탈로 배틀/이벤트/상점 방 이동)
- 각 방마다 1~8개 랜덤 포탈 생성, 층 수에 따라 난이도 조정
- 포탈 이동 시 층 수 증가, 패배 시 초기화

### 주사위 시스템
- 공격/스킬 선택 후 주사위 굴림으로 데미지 배율 결정
- 배율 공식: `Multiplier = 1.0 + FinalFace × 0.1` (예: -3→×0.7, +3→×1.3)
- 유물로 주사위 면 보정 가능

### 전투 시스템
- Speed 기반 턴 순서 자동 정렬
- EBattlePhase 상태머신으로 전투 흐름 관리
- 스태미나 충전 시스템 (매 턴 +20, 피격 시 +5)

### GAS 연동
- Custom Execution Calculation으로 데미지 공식 구현 (물리/마법 분리, 방어력 감소, 크리티컬)
- `IncomingDamage` 메타 어트리뷰트로 데미지 파이프라인 설계
- GameplayEffect로 비용(MP/Stamina) 및 초기 스탯 적용

### 어빌리티
- Melee/Ranged/Impact 타입 (캐릭터 이동, 이펙트, 투사체+컷씬 카메라)
- AnimNotify로 데미지 적용 타이밍 제어
- 다단히트, 레벨 해금, MP/Stamina 비용 시스템 지원

### UI & 입력
- **키보드 전용 조작**
  - **월드 탐험**: WASD 이동 / Space 점프 / F 상호작용(포탈)
  - **전투 메뉴**: ↑↓ 메뉴 이동 / ←→ 타겟 선택 / Enter/Space 확인 / Esc/Backspace 취소
- 턴 순서 패널, HP/MP/Stamina 상태 바, 플로팅 데미지 텍스트
- 주사위 롤링 오버레이, 승리/패배 위젯

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
