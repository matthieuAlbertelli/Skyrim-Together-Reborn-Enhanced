export type CharacterCreationPhase =
  | 'inactive'
  | 'openingRaceMenu'
  | 'editingRace'
  | 'raceReview'
  | 'classSelection'
  | 'loadoutSelection'
  | 'buildSummary'
  | 'buildConfirmed'
  | 'error';

export type CharacterClassId =
  | 'class.warrior'
  | 'class.mage'
  | 'class.thief';

export type CharacterClassTheme = 'warrior' | 'mage' | 'thief';

export interface CharacterClassDefinition {
  id: CharacterClassId;
  name: string;
  subtitle: string;
  description: string;
  majorSkills: readonly string[];
  minorSkills: readonly string[];
  roleTags: readonly string[];
  tarotTitle: string;
  archetype: string;
  loadoutHint: string;
  cooperativeGift: string;
  theme: CharacterClassTheme;
}

export type LoadoutGroupKind = 'choice' | 'announcement' | 'deferred';

export interface LoadoutGroupCondition {
  groupId: string;
  optionId: string;
}

export interface LoadoutRewardLine {
  name: string;
  quantity?: number;
  detail?: string;
  previewKey?: string;
  previewLabel?: string;
}

export interface LoadoutOptionDefinition {
  id: string;
  name: string;
  subtitle: string;
  description: string;
  rewards: readonly LoadoutRewardLine[];
  previewKey?: string;
  previewLabel?: string;
}

export interface LoadoutGroupDefinition {
  id: string;
  skill: string;
  title: string;
  kind: LoadoutGroupKind;
  required: boolean;
  description: string;
  rewards?: readonly LoadoutRewardLine[];
  options?: readonly LoadoutOptionDefinition[];
  previewKey?: string;
  previewLabel?: string;
  when?: LoadoutGroupCondition;
}

export interface CharacterLoadoutDefinition {
  classId: CharacterClassId;
  title: string;
  introduction: string;
  groups: readonly LoadoutGroupDefinition[];
}

export interface CharacterCreationViewState {
  visible: boolean;
  phase: CharacterCreationPhase;
  controlsLocked: boolean;
  raceConfirmed: boolean;
  raceFormId: number;
  raceName: string;
  selectedClassId: CharacterClassId | '';
  classConfirmed: boolean;
  selectedLoadoutOptions: Readonly<Record<string, string>>;
  loadoutConfirmed: boolean;
  buildConfirmed: boolean;
  serverPending: boolean;
  serverAuthoritative: boolean;
  error: string;
}
