import { CommonModule } from '@angular/common';
import {
  Component,
  HostListener,
  OnDestroy,
  OnInit,
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
  public selectedItemId: string | null = null;
  public selectedQuantity = 1;

  private readonly subscription = new Subscription();

  public constructor(
    private readonly trade: TradeUiService,
    private readonly uiSurface: UiSurfaceService,
  ) {}

  public ngOnInit(): void {
    this.subscription.add(
      this.trade.stateChange.subscribe(state => {
        const previousSessionId = this.session?.sessionId;
        this.state = state;

        if (!this.isSession(state)) {
          this.selectedItemId = null;
          this.selectedQuantity = 1;
          return;
        }

        if (previousSessionId !== state.sessionId) {
          this.activeCategory = 'all';
          this.selectedItemId = null;
          this.selectedQuantity = 1;
        }

        const selected = state.inventory.find(
          item => item.id === this.selectedItemId,
        );

        if (!selected) {
          this.selectItem(this.filteredInventory[0] ?? null);
        } else {
          this.selectedQuantity = this.clampQuantity(
            this.selectedQuantity,
            selected,
          );
        }
      }),
    );
  }

  public ngOnDestroy(): void {
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

  public get selectedItem(): TradeItem | null {
    const session = this.session;
    if (!session) {
      return null;
    }

    return (
      session.inventory.find(item => item.id === this.selectedItemId) ??
      this.filteredInventory[0] ??
      null
    );
  }

  public get selectedOfferedQuantity(): number {
    return this.selectedItem?.offered ?? 0;
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
    return Boolean(
      session &&
        session.mutable &&
        !session.localConfirmed,
    );
  }

  public selectCategory(category: TradeCategory): void {
    this.activeCategory = category;
    this.selectItem(this.filteredInventory[0] ?? null);
  }

  public selectItem(item: TradeItem | null): void {
    this.selectedItemId = item?.id ?? null;
    this.selectedQuantity = item
      ? this.clampQuantity(item.offered || 1, item)
      : 1;
  }

  public decreaseQuantity(): void {
    this.selectedQuantity = Math.max(1, this.selectedQuantity - 1);
  }

  public increaseQuantity(): void {
    const item = this.selectedItem;
    if (!item) {
      return;
    }

    this.selectedQuantity = Math.min(
      item.quantity,
      this.selectedQuantity + 1,
    );
  }

  public addSelectedItem(): void {
    const item = this.selectedItem;
    const session = this.session;
    if (!item || !session?.mutable) {
      return;
    }

    this.trade.setOffer(
      item.id,
      this.clampQuantity(this.selectedQuantity, item),
    );
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

  public trackByItemId(_index: number, item: TradeItem): string {
    return item.id;
  }

  private clampQuantity(quantity: number, item: TradeItem): number {
    return Math.max(1, Math.min(item.quantity, Math.trunc(quantity) || 1));
  }

  private isSession(state: TradeViewState): state is TradeSessionViewState {
    return state.visible && state.mode === 'session';
  }
}
