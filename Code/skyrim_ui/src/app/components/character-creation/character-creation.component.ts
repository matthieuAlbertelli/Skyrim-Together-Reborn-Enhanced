import { CommonModule } from '@angular/common';
import {
  Component,
  ElementRef,
  HostListener,
  OnDestroy,
  OnInit,
  ViewChild,
} from '@angular/core';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';
import {
  faArrowLeft,
  faBookOpen,
  faBoxOpen,
  faCheck,
  faCircleExclamation,
  faFeatherPointed,
  faHammer,
  faHatWizard,
  faListCheck,
  faLock,
  faRotateLeft,
  faShieldHalved,
  faUserPen,
  faUserSecret,
} from '@fortawesome/free-solid-svg-icons';
import { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import { Subscription } from 'rxjs';
import { CHARACTER_CLASSES } from '../../data/character-classes';
import {
  CharacterRaceDefinition,
  getRaceDefinition,
} from '../../data/character-races';
import { getLoadoutForClass } from '../../data/character-loadouts';
import {
  CharacterClassDefinition,
  CharacterClassId,
  CharacterCreationViewState,
  CharacterLoadoutDefinition,
  LoadoutGroupDefinition,
  LoadoutOptionDefinition,
  LoadoutRewardLine,
} from '../../models/character-creation';
import { CharacterCreationUiService } from '../../services/character-creation-ui.service';
import { UiSurfaceService } from '../../services/ui-surface.service';

interface SummaryDiscipline {
  id: string;
  name: string;
  domain: 'domain-warrior' | 'domain-mage' | 'domain-thief';
  groups: readonly LoadoutGroupDefinition[];
}

type SummaryFocus = 'race' | 'class' | 'discipline';

@Component({
  selector: 'app-character-creation',
  standalone: true,
  imports: [CommonModule, FontAwesomeModule],
  templateUrl: './character-creation.component.html',
  styleUrls: ['./character-creation.component.scss'],
})
export class CharacterCreationComponent implements OnInit, OnDestroy {
  public readonly confirmIcon = faCheck;
  public readonly editIcon = faUserPen;
  public readonly retryIcon = faRotateLeft;
  public readonly warningIcon = faCircleExclamation;
  public readonly shieldIcon = faShieldHalved;
  public readonly majorIcon = faHammer;
  public readonly minorIcon = faFeatherPointed;
  public readonly loreIcon = faBookOpen;
  public readonly packageIcon = faBoxOpen;
  public readonly checklistIcon = faListCheck;
  public readonly lockIcon = faLock;
  public readonly backIcon = faArrowLeft;

  public readonly classes = CHARACTER_CLASSES;

  public state: CharacterCreationViewState = {
    visible: false,
    phase: 'inactive',
    controlsLocked: false,
    raceConfirmed: false,
    raceFormId: 0,
    raceName: '',
    selectedClassId: '',
    classConfirmed: false,
    selectedLoadoutOptions: {},
    loadoutConfirmed: false,
    buildConfirmed: false,
    serverPending: false,
    serverAuthoritative: false,
    error: '',
  };

  public readonly surface$ = this.uiSurface.surfaceChange.asObservable();
  public activeGroupId = '';
  public previewLabel = '';
  public previewOrigin = '';
  public previewDescription = '';
  public previewKey = '';
  public previewIsSpell = false;
  public summaryFocus: SummaryFocus = 'race';
  public summaryDisciplineId = '';

  private readonly subscription = new Subscription();
  private previewViewportElement: HTMLElement | null = null;
  private previewResizeObserver: ResizeObserver | null = null;
  private previewRegionFrame: number | null = null;
  private lastPreviewRegionSignature = '';
  private lastPreviewKey = '';
  private pendingActiveGroupId = '';

  public constructor(
    private readonly characterCreation: CharacterCreationUiService,
    private readonly uiSurface: UiSurfaceService,
  ) {}

  @ViewChild('loadoutPreviewViewport')
  public set loadoutPreviewViewport(
    viewport: ElementRef<HTMLElement> | undefined,
  ) {
    this.previewResizeObserver?.disconnect();
    this.previewResizeObserver = null;
    this.previewViewportElement = viewport?.nativeElement ?? null;

    if (this.previewViewportElement && typeof ResizeObserver !== 'undefined') {
      this.previewResizeObserver = new ResizeObserver(() => {
        this.queuePreviewRegionUpdate();
      });
      this.previewResizeObserver.observe(this.previewViewportElement);
    }

    this.queuePreviewRegionUpdate();
  }

  public ngOnInit(): void {
    this.subscription.add(
      this.characterCreation.stateChange.subscribe(state => {
        const previousPhase = this.state.phase;
        const previousClassId = this.state.selectedClassId;
        this.state = state;

        const loadoutVisible = state.phase === 'loadoutSelection';
        const summaryVisible = state.phase === 'buildSummary';
        const previewVisible = loadoutVisible || summaryVisible;
        const previewWasVisible =
          previousPhase === 'loadoutSelection' ||
          previousPhase === 'buildSummary';

        if (loadoutVisible) {
          const pendingGroupVisible =
            this.pendingActiveGroupId &&
            this.visibleGroups.some(
              group => group.id === this.pendingActiveGroupId,
            );

          if (pendingGroupVisible) {
            this.activeGroupId = this.pendingActiveGroupId;
            this.pendingActiveGroupId = '';
          } else if (
            previousClassId !== state.selectedClassId ||
            !this.visibleGroups.some(group => group.id === this.activeGroupId)
          ) {
            this.activeGroupId = this.findInitialGroupId();
          }

          this.queuePreviewRegionUpdate();
          this.refreshPreview();
        } else if (summaryVisible) {
          if (previousPhase !== 'buildSummary') {
            this.summaryFocus = 'race';
            this.summaryDisciplineId = '';
          }
          this.queuePreviewRegionUpdate();
          this.refreshSummaryPreview();
        } else if (previewWasVisible && !previewVisible) {
          this.clearPreviewSurface();
        }
      }),
    );
  }

  public ngOnDestroy(): void {
    this.clearPreviewSurface();
    this.previewResizeObserver?.disconnect();
    this.previewResizeObserver = null;
    this.previewViewportElement = null;

    if (this.previewRegionFrame !== null) {
      window.cancelAnimationFrame(this.previewRegionFrame);
      this.previewRegionFrame = null;
    }

    this.subscription.unsubscribe();
  }

  public get selectedClass(): CharacterClassDefinition {
    return (
      this.classes.find(entry => entry.id === this.state.selectedClassId) ??
      this.classes[0]
    );
  }

  public get currentLoadout(): CharacterLoadoutDefinition {
    return getLoadoutForClass(this.state.selectedClassId);
  }

  public get selectedRace(): CharacterRaceDefinition {
    return getRaceDefinition(this.state.raceFormId, this.state.raceName);
  }

  public get summaryDisciplines(): readonly SummaryDiscipline[] {
    const disciplines: SummaryDiscipline[] = [];

    for (const group of this.visibleGroups) {
      const name = group.skill.split('·')[0].trim();
      const id = name.toLocaleLowerCase('fr-FR').replace(/[^a-z0-9]+/g, '-');
      const existing = disciplines.find(entry => entry.id === id);
      if (existing) {
        existing.groups = [...existing.groups, group];
      } else {
        disciplines.push({
          id,
          name,
          domain: this.skillDomain(group),
          groups: [group],
        });
      }
    }

    return disciplines;
  }

  public get activeSummaryDiscipline(): SummaryDiscipline | null {
    return (
      this.summaryDisciplines.find(
        discipline => discipline.id === this.summaryDisciplineId,
      ) ?? null
    );
  }

  public get summaryTitle(): string {
    if (this.summaryFocus === 'race') {
      return this.selectedRace.name;
    }
    if (this.summaryFocus === 'class') {
      return this.selectedClass.name;
    }
    return this.activeSummaryDiscipline?.name ?? this.selectedClass.name;
  }

  public get summaryEpithet(): string {
    if (this.summaryFocus === 'race') {
      return this.selectedRace.epithet;
    }
    if (this.summaryFocus === 'class') {
      return this.selectedClass.subtitle;
    }

    const choices = this.summaryChoiceNames;
    return choices.length ? choices.join(' · ') : 'Discipline du paquetage';
  }

  public get summaryDescription(): string {
    if (this.summaryFocus === 'race') {
      return this.selectedRace.description;
    }
    if (this.summaryFocus === 'class') {
      return this.selectedClass.description;
    }
    return this.activeSummaryDiscipline?.groups[0]?.description ?? '';
  }

  public get summaryHistory(): string {
    if (this.summaryFocus === 'race') {
      return this.selectedRace.history;
    }
    if (this.summaryFocus === 'class') {
      return `${this.selectedClass.loadoutHint} ${this.selectedClass.cooperativeGift}`;
    }

    const descriptions = this.activeSummaryDiscipline?.groups
      .map(group => this.selectedOptionFor(group)?.description)
      .filter((description): description is string => Boolean(description)) ?? [];
    return descriptions.join(' ');
  }

  public get summaryTags(): readonly string[] {
    if (this.summaryFocus === 'race') {
      return this.selectedRace.bonuses;
    }
    if (this.summaryFocus === 'class') {
      return [
        ...this.selectedClass.majorSkills,
        ...this.selectedClass.minorSkills,
      ];
    }
    return this.summaryChoiceNames;
  }

  public get summaryChoiceNames(): readonly string[] {
    const discipline = this.activeSummaryDiscipline;
    if (!discipline) {
      return [];
    }

    return discipline.groups
      .map(group => {
        if (group.kind === 'announcement') {
          return group.title;
        }
        return this.selectedOptionFor(group)?.name ?? group.title;
      })
      .filter((name, index, names) => names.indexOf(name) === index);
  }

  public get summaryRewards(): readonly LoadoutRewardLine[] {
    const discipline = this.activeSummaryDiscipline;
    if (!discipline) {
      return [];
    }

    return discipline.groups.reduce<LoadoutRewardLine[]>((rewards, group) => {
      const groupRewards =
        group.kind === 'announcement'
          ? group.rewards ?? []
          : this.selectedOptionFor(group)?.rewards ?? [];

      rewards.push(...groupRewards);
      return rewards;
    }, []);
  }

  public get visibleGroups(): readonly LoadoutGroupDefinition[] {
    return this.currentLoadout.groups.filter(group => this.groupVisible(group));
  }

  public get activeGroup(): LoadoutGroupDefinition {
    return (
      this.visibleGroups.find(group => group.id === this.activeGroupId) ??
      this.visibleGroups[0] ??
      this.currentLoadout.groups[0]
    );
  }

  public get activeOption(): LoadoutOptionDefinition | null {
    const group = this.activeGroup;
    const optionId = this.selectionFor(group);
    return group.options?.find(option => option.id === optionId) ?? null;
  }

  public get displayedOption(): LoadoutOptionDefinition | null {
    return this.activeOption ?? this.activeGroup.options?.[0] ?? null;
  }

  public get requiredChoiceGroups(): readonly LoadoutGroupDefinition[] {
    return this.visibleGroups.filter(
      group => group.kind === 'choice' && group.required,
    );
  }

  public get completedRequiredChoiceCount(): number {
    return this.requiredChoiceGroups.filter(group => this.selectionFor(group)).length;
  }

  public get loadoutComplete(): boolean {
    return (
      this.requiredChoiceGroups.length > 0 &&
      this.completedRequiredChoiceCount === this.requiredChoiceGroups.length
    );
  }

  public skillDomain(
    group: LoadoutGroupDefinition,
  ): 'domain-warrior' | 'domain-mage' | 'domain-thief' {
    const skill = group.skill.split('·')[0].trim().toLocaleLowerCase('fr-FR');

    if (
      ['altération', 'destruction', 'enchantement', 'illusion', 'invocation', 'restauration'].includes(skill)
    ) {
      return 'domain-mage';
    }

    if (
      ['alchimie', 'armure légère', 'crochetage', 'éloquence', 'furtivité', 'vol à la tire'].includes(skill)
    ) {
      return 'domain-thief';
    }

    return 'domain-warrior';
  }

  public classIcon(entry: CharacterClassDefinition): IconDefinition {
    switch (entry.id) {
      case 'class.mage':
        return faHatWizard;
      case 'class.thief':
        return faUserSecret;
      case 'class.warrior':
      default:
        return faShieldHalved;
    }
  }

  public selectClass(classId: CharacterClassId): void {
    if (this.state.selectedClassId === classId) {
      return;
    }

    this.characterCreation.selectClass(classId);
  }

  public confirmClass(): void {
    if (!this.state.selectedClassId) {
      return;
    }

    this.characterCreation.confirmClass();
  }

  public modifyRace(): void {
    this.characterCreation.modifyRace();
  }

  public confirmRace(): void {
    this.characterCreation.confirmRace();
  }

  public reopenClassSelection(): void {
    this.characterCreation.reopenClassSelection();
  }

  public selectLoadoutGroup(group: LoadoutGroupDefinition): void {
    if (this.activeGroupId === group.id) {
      return;
    }

    this.activeGroupId = group.id;
    this.refreshPreview();
  }

  public selectLoadoutOption(
    group: LoadoutGroupDefinition,
    option: LoadoutOptionDefinition,
  ): void {
    if (group.kind !== 'choice') {
      return;
    }

    this.activeGroupId = group.id;

    const nextGroupId = this.nextWeaponChoiceGroupId(group, option);
    if (nextGroupId) {
      this.pendingActiveGroupId = nextGroupId;
    }

    this.characterCreation.selectLoadoutOption(group.id, option.id);

    if (!nextGroupId) {
      this.previewOption(option, group);
    }
  }

  public previewRewardFromOption(
    event: Event,
    group: LoadoutGroupDefinition,
    option: LoadoutOptionDefinition,
    reward: LoadoutRewardLine,
  ): void {
    event.stopPropagation();
    this.selectLoadoutOption(group, option);
    this.previewReward(reward, option.name, option.description);
  }

  public previewRewardFromAnnouncement(
    event: Event,
    reward: LoadoutRewardLine,
  ): void {
    event.stopPropagation();
    this.previewReward(reward, this.activeGroup.title, this.activeGroup.description);
  }

  public rewardPreviewAvailable(reward: LoadoutRewardLine): boolean {
    return Boolean(reward.previewKey);
  }

  public isRewardSelected(reward: LoadoutRewardLine): boolean {
    const label = reward.previewLabel ?? reward.name;
    return this.previewLabel === label;
  }

  public showRaceSummary(): void {
    this.summaryFocus = 'race';
    this.summaryDisciplineId = '';
    this.refreshSummaryPreview();
  }

  public showClassSummary(): void {
    this.summaryFocus = 'class';
    this.summaryDisciplineId = '';
    this.refreshSummaryPreview();
  }

  public showDisciplineSummary(discipline: SummaryDiscipline): void {
    this.summaryFocus = 'discipline';
    this.summaryDisciplineId = discipline.id;
    this.refreshSummaryPreview();
  }

  public previewSummaryReward(
    event: Event,
    reward: LoadoutRewardLine,
  ): void {
    event.stopPropagation();
    this.previewReward(
      reward,
      this.activeSummaryDiscipline?.name ?? this.selectedClass.name,
      this.summaryDescription,
    );
  }

  public confirmLoadout(): void {
    if (!this.loadoutComplete) {
      return;
    }

    this.characterCreation.confirmLoadout();
  }

  public reopenLoadoutSelection(): void {
    this.characterCreation.reopenLoadoutSelection();
  }

  public confirmBuild(): void {
    this.characterCreation.confirmBuild();
  }

  public retryRaceMenu(): void {
    this.characterCreation.retryRaceMenu();
  }

  public recoverControls(): void {
    this.characterCreation.recoverControls();
  }

  public selectionFor(group: LoadoutGroupDefinition): string {
    return this.state.selectedLoadoutOptions[group.id] ?? '';
  }

  public selectedOptionFor(group: LoadoutGroupDefinition): LoadoutOptionDefinition | null {
    const optionId = this.selectionFor(group);
    return group.options?.find(option => option.id === optionId) ?? null;
  }

  public groupStatus(group: LoadoutGroupDefinition): string {
    if (group.kind === 'announcement') {
      return 'Accordé';
    }

    if (group.kind === 'deferred') {
      return 'À trancher';
    }

    return this.selectionFor(group) ? 'Scellé' : 'À choisir';
  }

  public groupComplete(group: LoadoutGroupDefinition): boolean {
    return (
      group.kind === 'announcement' ||
      group.kind === 'deferred' ||
      Boolean(this.selectionFor(group))
    );
  }

  public trackClass(_index: number, entry: CharacterClassDefinition): CharacterClassId {
    return entry.id;
  }

  public trackGroup(_index: number, group: LoadoutGroupDefinition): string {
    return group.id;
  }

  public trackOption(_index: number, option: LoadoutOptionDefinition): string {
    return option.id;
  }

  public trackReward(_index: number, reward: LoadoutRewardLine): string {
    return `${reward.name}|${reward.quantity ?? ''}|${reward.detail ?? ''}|${reward.previewKey ?? ''}`;
  }

  @HostListener('window:keydown', ['$event'])
  public handleKeyboard(event: KeyboardEvent): void {
    if (this.uiSurface.surfaceChange.getValue() !== 'characterCreation') {
      return;
    }

    if (this.state.serverPending) {
      event.preventDefault();
      return;
    }

    if (this.state.phase === 'classSelection') {
      this.handleClassKeyboard(event);
      return;
    }

    if (this.state.phase === 'loadoutSelection') {
      this.handleLoadoutKeyboard(event);
      return;
    }

    if (this.state.phase === 'buildSummary' && event.key === 'Enter') {
      event.preventDefault();
      this.confirmBuild();
    }
  }

  private handleClassKeyboard(event: KeyboardEvent): void {
    const currentIndex = Math.max(
      0,
      this.classes.findIndex(entry => entry.id === this.state.selectedClassId),
    );

    if (event.key === 'ArrowUp' || event.key === 'ArrowLeft') {
      event.preventDefault();
      const nextIndex = (currentIndex - 1 + this.classes.length) % this.classes.length;
      this.selectClass(this.classes[nextIndex].id);
      return;
    }

    if (event.key === 'ArrowDown' || event.key === 'ArrowRight') {
      event.preventDefault();
      const nextIndex = (currentIndex + 1) % this.classes.length;
      this.selectClass(this.classes[nextIndex].id);
      return;
    }

    if (event.key === 'Enter') {
      event.preventDefault();
      this.confirmClass();
    }
  }

  private handleLoadoutKeyboard(event: KeyboardEvent): void {
    const groups = this.visibleGroups;
    const currentGroupIndex = Math.max(
      0,
      groups.findIndex(group => group.id === this.activeGroupId),
    );

    if (event.key === 'ArrowUp' || event.key === 'ArrowDown') {
      event.preventDefault();
      const direction = event.key === 'ArrowUp' ? -1 : 1;
      const nextIndex = (currentGroupIndex + direction + groups.length) % groups.length;
      this.selectLoadoutGroup(groups[nextIndex]);
      return;
    }

    const options = this.activeGroup.options ?? [];
    if ((event.key === 'ArrowLeft' || event.key === 'ArrowRight') && options.length) {
      event.preventDefault();
      const selectedId = this.selectionFor(this.activeGroup);
      const selectedIndex = Math.max(
        0,
        options.findIndex(option => option.id === selectedId),
      );
      const direction = event.key === 'ArrowLeft' ? -1 : 1;
      const nextIndex = (selectedIndex + direction + options.length) % options.length;
      this.selectLoadoutOption(this.activeGroup, options[nextIndex]);
      return;
    }

    if (event.key === 'Enter') {
      event.preventDefault();

      if (this.activeGroup.kind === 'choice' && options.length) {
        const selected = this.activeOption ?? options[0];
        this.selectLoadoutOption(this.activeGroup, selected);
        return;
      }

      if (this.loadoutComplete) {
        this.confirmLoadout();
      }
    }
  }

  private nextWeaponChoiceGroupId(
    group: LoadoutGroupDefinition,
    option: LoadoutOptionDefinition,
  ): string {
    const separator = group.id.indexOf('.one_handed_');
    if (separator <= 0) {
      return '';
    }

    const prefix = group.id.slice(0, separator);
    let nextGroupId = '';

    if (group.id.endsWith('.one_handed_mode')) {
      if (option.id.endsWith('.one_handed_mode.steel')) {
        nextGroupId = `${prefix}.one_handed_steel`;
      } else if (option.id.endsWith('.one_handed_mode.dual_iron')) {
        nextGroupId = `${prefix}.one_handed_iron_main`;
      }
    } else if (group.id.endsWith('.one_handed_iron_main')) {
      nextGroupId = `${prefix}.one_handed_iron_off`;
    }

    return this.currentLoadout.groups.some(
      candidate => candidate.id === nextGroupId,
    )
      ? nextGroupId
      : '';
  }

  private groupVisible(group: LoadoutGroupDefinition): boolean {
    if (!group.when) {
      return true;
    }

    return this.state.selectedLoadoutOptions[group.when.groupId] === group.when.optionId;
  }

  private findInitialGroupId(): string {
    const incompleteRequired = this.visibleGroups.find(
      group => group.kind === 'choice' && group.required && !this.selectionFor(group),
    );

    return (
      incompleteRequired?.id ??
      this.visibleGroups.find(group => group.kind === 'choice')?.id ??
      this.visibleGroups[0]?.id ??
      ''
    );
  }

  private refreshPreview(): void {
    const group = this.activeGroup;

    if (group.kind === 'announcement') {
      this.previewRewardCollection(
        group.rewards ?? [],
        group.previewKey ?? '',
        group.previewLabel ?? group.title,
        group.description,
        group.title,
      );
      return;
    }

    if (group.kind === 'choice') {
      const option = this.selectedOptionFor(group) ?? group.options?.[0] ?? null;
      if (option) {
        this.previewOption(option, group);
        return;
      }
    }

    this.setPreviewState(
      group.previewKey ?? '',
      group.previewLabel ?? group.title,
      group.description,
      group.skill,
    );
    this.publishPreview(group.previewKey ?? '');
  }

  private previewOption(
    option: LoadoutOptionDefinition,
    group: LoadoutGroupDefinition,
  ): void {
    this.previewRewardCollection(
      option.rewards,
      option.previewKey ?? group.previewKey ?? '',
      option.previewLabel ?? option.name,
      option.description,
      option.name,
    );
  }

  private previewRewardCollection(
    rewards: readonly LoadoutRewardLine[],
    fallbackPreviewKey: string,
    fallbackLabel: string,
    fallbackDescription: string,
    origin: string,
  ): void {
    const previewableReward = rewards.find(reward => reward.previewKey);
    if (previewableReward) {
      this.previewReward(previewableReward, origin, fallbackDescription);
      return;
    }

    this.setPreviewState(
      fallbackPreviewKey,
      fallbackLabel,
      fallbackDescription,
      origin,
    );
    this.publishPreview(fallbackPreviewKey);
  }

  private previewReward(
    reward: LoadoutRewardLine,
    origin: string,
    fallbackDescription: string,
  ): void {
    const previewKey = reward.previewKey ?? '';
    const description = reward.detail ?? fallbackDescription;
    this.previewIsSpell =
      !previewKey && this.selectedClass.id === 'class.mage';
    this.setPreviewState(
      previewKey,
      reward.previewLabel ?? reward.name,
      description,
      origin,
    );
    this.publishPreview(previewKey);
  }

  private setPreviewState(
    previewKey: string,
    label: string,
    description: string,
    origin: string,
  ): void {
    this.previewKey = previewKey;
    if (previewKey) {
      this.previewIsSpell = false;
    }
    this.previewLabel = label;
    this.previewDescription = description;
    this.previewOrigin = origin;
  }

  private refreshSummaryPreview(): void {
    if (this.summaryFocus !== 'discipline') {
      this.previewIsSpell = false;
      this.setPreviewState('', this.summaryTitle, this.summaryDescription, this.summaryEpithet);
      this.publishPreview('');
      return;
    }

    const rewards = this.summaryRewards;
    const previewable = rewards.find(reward => reward.previewKey);
    if (previewable) {
      this.previewReward(
        previewable,
        this.activeSummaryDiscipline?.name ?? this.selectedClass.name,
        this.summaryDescription,
      );
      return;
    }

    const firstReward = rewards[0];
    if (firstReward) {
      this.previewReward(
        firstReward,
        this.activeSummaryDiscipline?.name ?? this.selectedClass.name,
        this.summaryDescription,
      );
      return;
    }

    this.previewIsSpell = false;
    this.setPreviewState('', this.summaryTitle, this.summaryDescription, this.summaryEpithet);
    this.publishPreview('');
  }

  private publishPreview(previewKey: string): void {
    if (previewKey === this.lastPreviewKey) {
      return;
    }

    this.lastPreviewKey = previewKey;
    if (previewKey) {
      this.characterCreation.previewLoadoutItem(previewKey);
    } else {
      this.characterCreation.clearLoadoutPreview();
    }
  }

  private clearPreviewSurface(): void {
    this.previewKey = '';
    this.previewLabel = '';
    this.previewDescription = '';
    this.previewOrigin = '';
    this.previewIsSpell = false;
    this.lastPreviewKey = '';
    this.lastPreviewRegionSignature = '';
    this.characterCreation.clearLoadoutPreview();
    this.characterCreation.setLoadoutPreviewRegion(0, 0, 0, 0);
  }

  private queuePreviewRegionUpdate(): void {
    if (this.previewRegionFrame !== null) {
      return;
    }

    this.previewRegionFrame = window.requestAnimationFrame(() => {
      this.previewRegionFrame = null;
      this.publishPreviewRegion();
    });
  }

  private publishPreviewRegion(): void {
    const element = this.previewViewportElement;
    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const previewVisible =
      this.state.phase === 'loadoutSelection' ||
      this.state.phase === 'buildSummary';

    if (!previewVisible || !element || viewportWidth <= 0 || viewportHeight <= 0) {
      this.publishPreviewRegionValues(0, 0, 0, 0);
      return;
    }

    const rect = element.getBoundingClientRect();
    if (rect.width <= 1 || rect.height <= 1) {
      this.publishPreviewRegionValues(0, 0, 0, 0);
      return;
    }

    this.publishPreviewRegionValues(
      rect.left / viewportWidth,
      rect.top / viewportHeight,
      rect.width / viewportWidth,
      rect.height / viewportHeight,
    );
  }

  private publishPreviewRegionValues(
    left: number,
    top: number,
    width: number,
    height: number,
  ): void {
    const signature = [left, top, width, height]
      .map(value => value.toFixed(6))
      .join(':');

    if (signature === this.lastPreviewRegionSignature) {
      return;
    }

    this.lastPreviewRegionSignature = signature;
    this.characterCreation.setLoadoutPreviewRegion(left, top, width, height);
  }
}
