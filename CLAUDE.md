# ProjectTB - CLAUDE.md

## 시스템 전체 흐름

### 게임 시작
```
UTBGameInstance::Init() → UAbilitySystemGlobals::Get().InitGlobalData()
```

### 월드 → 배틀씬 전환
```
적과 조우 → FBattleTransitionData(적 클래스, 복귀 위치) 저장 → OpenLevel("BattleLevel")
```

### 캐릭터 초기화 (BeginPlay)
```
ABattleCombatant::BeginPlay()
  → ASC->InitAbilityActorInfo(this, this)
  → ApplyStartingEffects()   // GE로 초기 스탯 세팅
  → GrantStartingAbilities() // 어빌리티 부여
```

### 전투 시작
```
BattleManager::StartBattle(Players, Enemies)
  → BuildRoundOrder()     // Speed 내림차순 정렬
  → SetPhase(BattleStart)
  → 1초 후 AdvanceTurn()
```

### 플레이어 턴
```
SetPhase(PlayerTurn)
  → HUD → BattleMenuWidget::ShowMainMenu()  // 키보드 포커스 이동
  → 키입력(↑↓Enter/Esc) → NavigateUp/Down / Confirm / Cancel
      → Attack   → PlayerSelectAttack()   → SetPhase(SelectingTarget)
      → Ability  → PlayerSelectAbility()  → SetPhase(SelectingTarget)
  → 타겟 선택(←→) → PlayerSelectTarget() → ExecuteAction()
```

### 어빌리티 실행 (GAS 체인)
```
ExecuteAction(Caster, Target, AbilityClass)
  → PendingTarget = Target
  → SetPhase(ExecutingAction)
  → ASC->TryActivateAbility()
      [Blueprint GA 내부]
      → PendingTarget 읽기
      → [Melee] 타겟으로 이동
      → PlayMontageAndWait
          → AnimNotify "OnHit/OnCast" → GE_Damage 적용
      → [Melee] 원위치 복귀
      → EndAbility() → BattleManager::OnActionComplete()
```

### 데미지 계산 (TBDamageExecution)
```
Reduction   = Defense / (Defense + 100)
BaseDamage  = Attack * AbilityMultiplier(SetByCaller)
FinalDamage = BaseDamage * (1 - Reduction) * Variance(0.9~1.1)
크리티컬(CriticalChance) → FinalDamage *= CriticalMultiplier
→ IncomingDamage 메타 어트리뷰트 출력
→ PostGameplayEffectExecute: HP -= IncomingDamage
→ HP <= 0 → OnDeathInternal() → Dead 태그, 어빌리티 취소, OnDeath 델리게이트
```

### 적 턴
```
SetPhase(EnemyTurn)
  → SelectAction() // 기본: 랜덤 타겟 + 첫 번째 어빌리티 (Blueprint 오버라이드 가능)
  → 0.5초 딜레이 → ExecuteAction() → 위와 동일
```

### UI 델리게이트 흐름
```
BattleManager 델리게이트 → TBBattleHUD 수신
  OnBattlePhaseChanged → BattleMenuWidget 제어 (열기/닫기/타겟선택)
  OnTurnBegin          → CharacterStatusWidget::SetActiveCharacter()
  OnTurnOrderUpdated   → TurnOrderWidget::UpdateTurnOrder()
```

---

## 프로젝트 개요
Unreal Engine 5.6 턴제 RPG. GAS(Gameplay Ability System) 기반.
스크린샷 레퍼런스: 좌측 턴 순서 패널, 키보드 배틀 메뉴, HP/MP/Stamina 하단 바.

---

## 기술 스택
- **Engine:** UE 5.6.1
- **언어:** C++ 위주 (Blueprint는 데이터/비주얼만)
- **어빌리티:** GAS (GameplayAbilities, GameplayTags, GameplayTasks)
- **입력:** Enhanced Input System
- **UI:** UMG (C++ 기반 클래스 + Blueprint 비주얼)

---

## 파일 구조

```
Source/ProjectTB/
├── Public/
│   ├── TBGameplayTags.h
│   ├── TBGameInstance.h
│   ├── Attributes/TBAttributeSet.h
│   ├── Abilities/TBGameplayAbility.h
│   ├── Battle/
│   │   ├── BattleCombatant.h
│   │   ├── BattlePlayerCharacter.h
│   │   ├── BattleEnemyCharacter.h
│   │   └── BattleManager.h
│   └── UI/
│       ├── TBBattleHUD.h
│       ├── TurnOrderWidget.h
│       ├── BattleMenuWidget.h
│       └── CharacterStatusWidget.h
└── Private/ (각 .cpp)
```

---

## 아키텍처 핵심 규칙

### GAS 설정
- ASC는 `ABattleCombatant`에 직접 보유 (PlayerState 아님)
- `InitAbilityActorInfo(this, this)` — BeginPlay에서 호출
- AttributeSet은 생성자에서 `CreateDefaultSubobject`로 생성
- `UAbilitySystemGlobals::Get().InitGlobalData()` — TBGameInstance에서 호출

### 어빌리티
- 기반: `UTBGameplayAbility` (C++)
- 개별 어빌리티: Blueprint 서브클래스로 생성 (GA_BasicAttack, GA_FlashAttacks 등)
- 비용: `CostType` (Stamina/MP/HP) + `CostAmount` 필드로 UI에 표시
- 타겟: `TargetType` (SingleEnemy/AllEnemies/SingleAlly/Self)

### 턴 순서
- `Speed` 속성 기준 내림차순 정렬 → 라운드 반복
- `CurrentRoundOrder[]` + `CurrentRoundIndex`로 관리
- 사망 시 제거 후 라운드 끝에서 재정렬
- `GetUpcomingTurns(N)` — UI 표시용 다음 N턴 미리 계산

### 상태머신 (EBattlePhase)
```
None → BattleStart → PlayerTurn ↔ SelectingTarget → ExecutingAction
                   → EnemyTurn                    → ExecutingAction
                   → BattleVictory / BattleDefeat
```

### 데미지 처리
- GameplayEffect가 `IncomingDamage` 메타 속성에 값 설정
- `PostGameplayEffectExecute`에서 `IncomingDamage` → HP 차감
- HP <= 0 → `BattleCombatant::OnDeathInternal()` 호출

---

## C++ / Blueprint 역할 분담

| 작업 | 담당 |
|------|------|
| AttributeSet, GAS 기반 로직 | C++ |
| 턴 순서, 상태머신 | C++ |
| UI 기반 클래스 (함수/델리게이트) | C++ |
| 개별 어빌리티 구현 | Blueprint |
| GameplayEffect 수치 | Blueprint |
| 캐릭터 메시, 애니메이션 | Blueprint |
| UI 레이아웃, 디자인 | Blueprint |

---

## GameplayTag 네임스페이스

```
Battle.Phase.*          → 전투 상태 (None/Start/PlayerTurn/EnemyTurn/Executing/Victory/Defeat)
Combatant.State.*       → 유닛 상태 (Dead/Stunned/Acting)
Ability.Type.*          → 어빌리티 종류 (Physical/Magic/Support)
Ability.Cost.*          → 비용 타입 (Stamina/MP/HP)
Effect.Damage.*         → 데미지 종류 (Physical/Magic)
Effect.Heal             → 힐
```

---

## 코딩 컨벤션
- 클래스 접두사: `A`(Actor), `U`(Object), `F`(Struct), `E`(Enum)
- 모듈 API 매크로: `PROJECTTB_API`
- 헤더: `#pragma once` 사용
- 델리게이트: `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*` 사용
- Attribute 접근자: `ATTRIBUTE_ACCESSORS` 매크로 사용

---

## 구현 순서 (참고)
1. `ProjectTB.Build.cs` — GAS, UMG 모듈 추가
2. `TBGameplayTags` — 전체 태그 정의
3. `TBAttributeSet` — 스탯 + 데미지 처리
4. `TBGameplayAbility` — 어빌리티 기반 클래스
5. `BattleCombatant` — ASC 세팅, 어빌리티 부여
6. `BattlePlayerCharacter` / `BattleEnemyCharacter`
7. `BattleManager` — 턴 시스템 + 상태머신
8. UI 기반 클래스 4개
9. `TBGameInstance` — GAS 전역 초기화

---

## 플레이어 캐릭터 (3종)

### 공통
- Blueprint 클래스: `BattlePlayerCharacter` 상속
- 초기 스탯: `GE_[클래스명]BaseStats` (Instant GE, Blueprint)
- 어빌리티: `StartingAbilities[]`에 등록

---

### 검사 (Swordsman)
**콘셉트:** 근접 탱커. 높은 HP/방어, 낮은 속도.

| 스탯 | 수치 |
|------|------|
| HP | 높음 |
| MP | 낮음 |
| Stamina | 중간 |
| Speed | 낮음 |
| PhysicalAttack | 높음 |
| PhysicalDefense | 높음 |
| MagicAttack | 낮음 |
| MagicDefense | 낮음 |

어빌리티: (미정)
- 기본 공격
- (스킬 추가 예정)

---

### 마법사 (Mage)
**콘셉트:** 광역 마법 딜러. 높은 MagicAttack, 낮은 HP/방어.

| 스탯 | 수치 |
|------|------|
| HP | 낮음 |
| MP | 높음 |
| Stamina | 낮음 |
| Speed | 중간 |
| PhysicalAttack | 낮음 |
| PhysicalDefense | 낮음 |
| MagicAttack | 높음 |
| MagicDefense | 높음 |

어빌리티: (미정)
- 기본 공격
- (스킬 추가 예정)

---

### 궁수 (Archer)
**콘셉트:** 원거리 딜러. 높은 Speed, 중간 PhysicalAttack.

| 스탯 | 수치 |
|------|------|
| HP | 중간 |
| MP | 낮음 |
| Stamina | 높음 |
| Speed | 높음 |
| PhysicalAttack | 중간 |
| PhysicalDefense | 낮음 |
| MagicAttack | 낮음 |
| MagicDefense | 중간 |

어빌리티: (미정)
- 기본 공격
- (스킬 추가 예정)

---

## 적 캐릭터

### 공통
- Blueprint 클래스: `BattleEnemyCharacter` 상속
- 초기 스탯: `GE_[적이름]BaseStats` (Instant GE, Blueprint)
- AI 행동: `SelectEnemyAction()` — Blueprint에서 오버라이드
- 초기 테스트: UE 기본 마네킹(Mannequin)으로 더미 적 세팅

### AI 행동 방식
- C++에서 기본 구현: 살아있는 플레이어 중 랜덤 타겟 + 기본 공격
- Blueprint 오버라이드로 적마다 고유 패턴 추가 가능
- 적 종류는 구상 중 → 추후 추가 예정

---

## 애니메이션 아키텍처

### 핵심 원칙
애니메이션 타입은 **캐릭터 클래스가 아닌 어빌리티 단위**로 결정된다.
마법사도 근접 어빌리티를 가질 수 있고, 궁수도 근접/원거리 혼용 가능.

### 어빌리티 타입 (EAbilityAnimType)

#### Melee — 캐릭터가 타겟에게 이동
```
1. 캐릭터 원위치 저장
2. 타겟 앞까지 이동 (Lerp)
3. 공격 AnimMontage 재생
4. AnimNotify "OnHit" → 데미지 GE 적용
5. 원위치 복귀
6. EndAbility() → BattleManager 턴 종료
```

#### Ranged — 캐릭터 제자리, 이펙트가 타겟으로
```
1. 캐스팅 AnimMontage 재생 (제자리)
2. AnimNotify "OnCast" → 타겟 위치에 이펙트 스폰
   (Niagara System 또는 별도 Actor — 추후 결정)
3. 이펙트 재생 완료 → 데미지 GE 적용
4. EndAbility() → BattleManager 턴 종료
```

### TBGameplayAbility 애니메이션 관련 필드
```
EAbilityAnimType AnimationType   // Melee / Ranged — 어빌리티마다 개별 설정
UAnimMontage* AbilityMontage     // 어빌리티 실행 몽타주
float MoveSpeed                  // Melee일 때 이동 속도
// 이펙트 (Niagara vs Actor 미정 → 추후 추가)
```

### AbilityTask 흐름
- **이동:** 커스텀 `UAbilityTask_MoveToTarget` (Lerp 이동, Melee만)
- **몽타주:** `UAbilityTask_PlayMontageAndWait` (UE 기본 제공)
- **OnHit/OnCast:** `AnimNotify`로 타이밍 제어
- **복귀:** Melee만 — 원위치 저장 후 Lerp 복귀

---

## 데미지 공식

### 물리 데미지
```
Reduction   = PhysicalDefense / (PhysicalDefense + 100)   // 0~1, 절대 1 불가
BaseDamage  = PhysicalAttack * AbilityMultiplier
FinalDamage = BaseDamage * (1 - Reduction) * RandomVariance(0.9~1.1)

크리티컬 발생 시: FinalDamage * 1.5
```

### 마법 데미지
```
동일 구조, MagicAttack / MagicDefense 사용
```

### 크리티컬
- **CriticalChance** — 별도 스탯 (Speed와 무관)
- **CriticalMultiplier** — 기본 1.5x (추후 조정 가능)
- Speed는 **턴 순서에만** 영향

### AttributeSet 스탯 목록 (최종)
| 스탯 | 역할 |
|------|------|
| HP / MaxHP | 체력 |
| MP / MaxMP | 마나 |
| Stamina / MaxStamina | 스태미나 |
| Speed | 턴 순서 결정 (크리티컬 무관) |
| PhysicalAttack | 물리 공격력 |
| MagicAttack | 마법 공격력 |
| PhysicalDefense | 물리 방어력 |
| MagicDefense | 마법 방어력 |
| CriticalChance | 크리티컬 확률 (0~1) |
| CriticalMultiplier | 크리티컬 배율 (기본 1.5) |
| IncomingDamage | (메타) 데미지 계산용 |
| IncomingHeal | (메타) 힐 계산용 |

---

## 전투 진입/종료

### 흐름
```
월드 (3인칭 탐험)
  → 적과 조우 (트리거 볼륨)
  → 페이드 아웃
  → OpenLevel("BattleLevel") — GameInstance에 파티/적 정보 저장
  → 배틀 씬: 캐릭터 스폰 → 전투
  → 전투 종료 (승/패)
  → 페이드 아웃 → 원래 월드 복귀 (저장된 위치)
```

### GameInstance 역할
- 레벨 간 데이터 유지 (파티 구성, 적 구성, 복귀 위치)
- GAS 전역 초기화 (`InitGlobalData`)

---

## UI 키 바인딩 (배틀씬)

| 키 | 동작 |
|----|------|
| ↑ / ↓ | 메뉴 항목 이동 |
| ← / → | 타겟 선택 시 이동 |
| Enter / Space | 확인 |
| Escape / Backspace | 취소 / 뒤로 |

- Enhanced Input으로 매핑
- 배틀씬 진입 시 입력 컨텍스트 전환 (월드 → 배틀 UI 전용)

---

## 카메라 동작 (배틀씬)

- 기본: 팀 전체가 보이는 고정 각도
- 턴 시작 시: 현재 액터로 카메라 이동 (부드러운 보간)
- 액션 실행 시: 공격자 추적
- 액션 완료 후: 기본 앵글로 복귀

---

## 데미지 숫자 UI

- 데미지/힐 발생 시 타겟 위치에 플로팅 텍스트 스폰
- `OnDamageReceived` 델리게이트 → UI에서 구독 → 텍스트 Actor 스폰
- 크리티컬 시 색상/크기 다르게 표시
- Blueprint로 구현 (C++에서 델리게이트만 정의)

---

## 상세 아키텍처
`C:\Users\JIWON\.claude\projects\d--Unreal-ProjectTB\memory\architecture.md` 참고
