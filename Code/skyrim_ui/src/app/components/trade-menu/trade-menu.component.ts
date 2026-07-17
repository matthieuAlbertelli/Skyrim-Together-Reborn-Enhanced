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
  IconDefinition,
  faArrowRightArrowLeft,
  faBookOpen,
  faBoxOpen,
  faCheck,
  faFlask,
  faGem,
  faHourglassHalf,
  faKhanda,
  faLock,
  faShieldHalved,
  faTriangleExclamation,
  faXmark,
} from '@fortawesome/free-solid-svg-icons';
import { Subscription } from 'rxjs';
import {
  TradeCategory,
  TradeInviteViewState,
  TradeItem,
  TradeOutgoingViewState,
  TradeSessionViewState,
  TradeViewState,
} from '../../models/trade';
import { TradeUiService } from '../../services/trade-ui.service';
import { UiSurfaceService } from '../../services/ui-surface.service';

interface CategoryOption {
  id: TradeCategory;
  label: string;
  icon: IconDefinition;
}

type PreviewSource = 'inventory' | 'local-offer' | 'remote-offer';

interface PreviewSelection {
  source: PreviewSource;
  itemId: string;
}

@Component({
  selector: 'app-trade-menu',
  standalone: true,
  imports: [CommonModule, FontAwesomeModule],
  templateUrl: './trade-menu.component.html',
  styleUrls: ['./trade-menu.component.scss'],
})
export class TradeMenuComponent implements OnInit, OnDestroy {
  public readonly exchangeIcon = faArrowRightArrowLeft;
  public readonly acceptIcon = faCheck;
  public readonly closeIcon = faXmark;
  public readonly lockIcon = faLock;
  public readonly waitIcon = faHourglassHalf;
  public readonly warningIcon = faTriangleExclamation;

  public readonly categories: CategoryOption[] = [
    { id: 'all', label: 'Tout', icon: faBoxOpen },
    { id: 'weapons', label: 'Armes', icon: faKhanda },
    { id: 'armor', label: 'Armures', icon: faShieldHalved },
    { id: 'consumables', label: 'Consommables', icon: faFlask },
    { id: 'books', label: 'Livres', icon: faBookOpen },
    { id: 'misc', label: 'Divers', icon: faGem },
  ];

  public state: TradeViewState = { visible: false };
  public readonly surface$ = this.uiSurface.surfaceChange.asObservable();
  public activeCategory: TradeCategory = 'all';
  public selectedQuantity = 1;
  public previewSelection: PreviewSelection | null = null;

  private readonly subscription = new Subscription();
  private completedDismissTimer: ReturnType<typeof setTimeout> | null = null;
  private previewViewportElement: HTMLElement | null = null;
  private previewResizeObserver: ResizeObserver | null = null;
  private previewRegionFrame: number | null = null;
  private lastPreviewRegionSignature = '';

  public constructor(
    private readonly trade: TradeUiService,
    private readonly uiSurface: UiSurfaceService,
  ) {}

  @ViewChild('previewViewport')
  public set previewViewport(
    viewport: ElementRef<HTMLElement> | undefined,
  ) {
    this.previewResizeObserver?.disconnect();
    this.previewResizeObserver = null;
    this.previewViewportElement = viewport?.nativeElement ?? null;

    if (
      this.previewViewportElement &&
      typeof ResizeObserver !== 'undefined'
    ) {
      this.previewResizeObserver = new ResizeObserver(() => {
        this.queuePreviewRegionUpdate();
      });
      this.previewResizeObserver.observe(this.previewViewportElement);
    }

    this.queuePreviewRegionUpdate();
  }

  public ngOnInit(): void {
    this.subscription.add(
      this.trade.stateChange.subscribe(state => {
        const previousSessionId = this.session?.sessionId;
        const previousPhase = this.session?.phase;
        this.state = state;
        this.queuePreviewRegionUpdate();

        if (!this.isSession(state)) {
          this.previewSelection = null;
          this.selectedQuantity = 1;
          this.trade.clearPreview();
          this.clearCompletedDismissTimer();
          return;
        }

        if (previousSessionId !== state.sessionId) {
          this.activeCategory = 'all';
          this.previewSelection = null;
          this.selectedQuantity = 1;
        }

        if (!this.resolvePreviewItem()) {
          this.selectPreview(this.filteredInventory[0] ?? null, 'inventory');
        } else {
          const inventoryItem = this.resolveInventoryItemForPreview();
          if (inventoryItem) {
            this.selectedQuantity = this.clampQuantity(
              this.selectedQuantity,
              inventoryItem,
            );
          }
        }

        if (state.phase === 'completed' && previousPhase !== 'completed') {
          this.scheduleCompletedDismiss();
        } else if (state.phase !== 'completed') {
          this.clearCompletedDismissTimer();
        }
      }),
    );
  }

  public ngOnDestroy(): void {
    this.trade.clearPreview();
    this.trade.setPreviewRegion(0, 0, 0, 0);
    this.previewResizeObserver?.disconnect();
    this.previewResizeObserver = null;
    this.previewViewportElement = null;
    if (this.previewRegionFrame !== null) {
      window.cancelAnimationFrame(this.previewRegionFrame);
      this.previewRegionFrame = null;
    }
    this.clearCompletedDismissTimer();
    this.subscription.unsubscribe();
  }

  @HostListener('window:keydown', ['$event'])
  public onKeyDown(event: KeyboardEvent): void {
    if (!this.state.visible || event.key !== 'Escape') {
      return;
    }

    event.preventDefault();
    event.stopPropagation();
    this.closeCurrentState();
  }

  public get invite(): TradeInviteViewState | null {
    return this.state.visible && this.state.mode === 'invite'
      ? this.state
      : null;
  }

  public get outgoing(): TradeOutgoingViewState | null {
    return this.state.visible && this.state.mode === 'outgoing'
      ? this.state
      : null;
  }

  public get session(): TradeSessionViewState | null {
    return this.isSession(this.state) ? this.state : null;
  }

  public get simpleError(): string {
    return 'error' in this.state ? this.state.error || '' : '';
  }

  public get filteredInventory(): TradeItem[] {
    const session = this.session;
    if (!session) {
      return [];
    }

    if (this.activeCategory === 'all') {
      return session.inventory;
    }

    return session.inventory.filter(
      item => item.category === this.activeCategory,
    );
  }

  public get previewItem(): TradeItem | null {
    return this.resolvePreviewItem();
  }

  public get phaseLabel(): string {
    switch (this.session?.phase) {
      case 'negotiating':
        return 'Négociation';
      case 'locked':
        return 'Vérification';
      case 'applying':
        return 'Transfert';
      case 'completed':
        return 'Terminé';
      case 'cancelled':
        return 'Annulé';
      case 'failed':
        return 'Échec';
      default:
        return 'Synchronisation';
    }
  }

  public get isTerminal(): boolean {
    const phase = this.session?.phase;
    return phase === 'completed' || phase === 'cancelled' || phase === 'failed';
  }

  public get canConfirm(): boolean {
    const session = this.session;
    return Boolean(session && session.mutable && !session.localConfirmed);
  }

  public selectCategory(category: TradeCategory): void {
    this.activeCategory = category;
    this.selectPreview(this.filteredInventory[0] ?? null, 'inventory');
  }

  public selectPreview(item: TradeItem | null, source: PreviewSource): void {
    this.previewSelection = item ? { source, itemId: item.id } : null;

    if (item) {
      this.trade.previewItem(item.id);
    } else {
      this.trade.clearPreview();
    }

    const inventoryItem = item
      ? this.session?.inventory.find(entry => entry.id === item.id) ?? null
      : null;

    this.selectedQuantity = inventoryItem
      ? this.clampQuantity(inventoryItem.offered || 1, inventoryItem)
      : 1;
  }

  public isPreviewed(item: TradeItem, source: PreviewSource): boolean {
    return (
      this.previewSelection?.source === source &&
      this.previewSelection.itemId === item.id
    );
  }

  public decreaseQuantity(): void {
    this.selectedQuantity = Math.max(1, this.selectedQuantity - 1);
  }

  public increaseQuantity(): void {
    const item = this.resolveInventoryItemForPreview();
    if (!item) {
      return;
    }

    this.selectedQuantity = Math.min(item.quantity, this.selectedQuantity + 1);
  }

  public addSelectedItem(): void {
    const item = this.resolveInventoryItemForPreview();
    if (!item || !this.session?.mutable) {
      return;
    }

    this.trade.setOffer(
      item.id,
      this.clampQuantity(this.selectedQuantity, item),
    );
  }

  public addOneToOffer(item: TradeItem): void {
    const inventoryItem = this.session?.inventory.find(
      entry => entry.id === item.id,
    );

    if (!inventoryItem || !this.session?.mutable) {
      return;
    }

    this.selectPreview(inventoryItem, 'inventory');
    const nextQuantity = Math.min(
      inventoryItem.quantity,
      Math.max(0, inventoryItem.offered) + 1,
    );
    this.selectedQuantity = Math.max(1, nextQuantity);
    this.trade.setOffer(inventoryItem.id, nextQuantity);
  }

  public removeFromOffer(itemId: string): void {
    if (!this.session?.mutable) {
      return;
    }

    this.trade.setOffer(itemId, 0);
  }

  public acceptInvite(): void {
    if (this.invite) {
      this.trade.acceptInvite(this.invite.sessionId);
    }
  }

  public rejectInvite(): void {
    if (this.invite) {
      this.trade.rejectInvite(this.invite.sessionId);
    }
  }

  public confirmTrade(): void {
    if (this.canConfirm) {
      this.trade.confirm();
    }
  }

  public cancelTrade(): void {
    if (this.session && !this.isTerminal) {
      this.trade.cancel();
    }
  }

  public closeCurrentState(): void {
    if (this.invite) {
      this.rejectInvite();
      return;
    }

    if (this.outgoing) {
      this.trade.dismissOutgoing();
      return;
    }

    if (!this.session) {
      return;
    }

    if (this.isTerminal) {
      this.trade.dismiss();
    } else {
      this.trade.cancel();
    }
  }

  public itemIcon(item: TradeItem): IconDefinition {
    switch (item.icon) {
      case 'weapon':
      case 'ammo':
        return faKhanda;
      case 'armor':
        return faShieldHalved;
      case 'ingredient':
      case 'potion':
        return faFlask;
      case 'book':
        return faBookOpen;
      default:
        return faGem;
    }
  }

  public itemColorClass(item: TradeItem): string {
    switch (item.icon) {
      case 'weapon':
        return 'item-tone--weapon';
      case 'ammo':
        return 'item-tone--ammo';
      case 'armor':
        return 'item-tone--armor';
      case 'ingredient':
        return 'item-tone--ingredient';
      case 'potion':
        return 'item-tone--potion';
      case 'book':
        return 'item-tone--book';
      default:
        return item.category === 'misc'
          ? 'item-tone--misc'
          : `item-tone--${item.category}`;
    }
  }

  public trackByItemId(_index: number, item: TradeItem): string {
    return item.id;
  }

  private resolvePreviewItem(): TradeItem | null {
    const session = this.session;
    const selection = this.previewSelection;
    if (!session || !selection) {
      return null;
    }

    const source =
      selection.source === 'inventory'
        ? session.inventory
        : selection.source === 'local-offer'
          ? session.localOffer
          : session.remoteOffer;

    return source.find(item => item.id === selection.itemId) ?? null;
  }

  private resolveInventoryItemForPreview(): TradeItem | null {
    const session = this.session;
    const selection = this.previewSelection;
    if (!session || !selection) {
      return null;
    }

    return session.inventory.find(item => item.id === selection.itemId) ?? null;
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

    if (
      !this.session ||
      !element ||
      viewportWidth <= 0 ||
      viewportHeight <= 0
    ) {
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
    this.trade.setPreviewRegion(left, top, width, height);
  }

  private scheduleCompletedDismiss(): void {
    this.clearCompletedDismissTimer();
    this.completedDismissTimer = setTimeout(() => {
      if (this.session?.phase === 'completed') {
        this.trade.dismiss();
      }
    }, 650);
  }

  private clearCompletedDismissTimer(): void {
    if (this.completedDismissTimer === null) {
      return;
    }

    clearTimeout(this.completedDismissTimer);
    this.completedDismissTimer = null;
  }

  private clampQuantity(quantity: number, item: TradeItem): number {
    return Math.max(1, Math.min(item.quantity, Math.trunc(quantity) || 1));
  }

  private isSession(state: TradeViewState): state is TradeSessionViewState {
    return state.visible && state.mode === 'session';
  }
}
