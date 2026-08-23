import { CommonModule } from '@angular/common';
import { Component, HostListener, OnDestroy, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { TranslocoModule } from '@ngneat/transloco';
import { Subscription } from 'rxjs';
import {
  CampaignBootstrapUiService,
  CampaignBootstrapViewState,
  DEFAULT_CAMPAIGN_BOOTSTRAP_STATE,
} from '../../services/campaign-bootstrap-ui.service';
import { StoreService } from '../../services/store.service';
import { UiSurfaceService } from '../../services/ui-surface.service';

const CODE_ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';

@Component({
  selector: 'app-campaign-bootstrap',
  standalone: true,
  imports: [CommonModule, FormsModule, TranslocoModule],
  templateUrl: './campaign-bootstrap.component.html',
  styleUrls: ['./campaign-bootstrap.component.scss'],
})
export class CampaignBootstrapComponent implements OnInit, OnDestroy {
  public state: CampaignBootstrapViewState = DEFAULT_CAMPAIGN_BOOTSTRAP_STATE;
  public address = '';
  public password = '';
  public pseudo = '';
  public code = '';
  public surface = 'none';

  private readonly subscriptions = new Subscription();

  public constructor(
    public readonly bootstrap: CampaignBootstrapUiService,
    private readonly uiSurface: UiSurfaceService,
    private readonly storeService: StoreService,
  ) {
    this.address = this.storeService.get('last_connected_address', '');
  }

  public ngOnInit(): void {
    this.subscriptions.add(
      this.bootstrap.stateChange.subscribe(state => {
        this.state = state;
        if (state.screen === 'join' && state.joinCode) {
          this.code = state.joinCode;
        }
      }),
    );
    this.subscriptions.add(
      this.uiSurface.surfaceChange.subscribe(surface => {
        this.surface = surface;
      }),
    );
  }

  public ngOnDestroy(): void {
    this.subscriptions.unsubscribe();
  }

  public get visible(): boolean {
    return this.surface === 'campaignBootstrap' && this.state.active;
  }

  public get validCode(): boolean {
    return this.code.length === 4;
  }

  public get validPseudo(): boolean {
    const pseudo = this.pseudo.trim();
    return (
      pseudo.length > 0 &&
      [...pseudo].length <= 24 &&
      !/[\u0000-\u001f\u007f-\u009f]/u.test(pseudo)
    );
  }

  public normalizeCode(value: string): void {
    this.code = value
      .toUpperCase()
      .split('')
      .filter(character => CODE_ALPHABET.includes(character))
      .slice(0, 4)
      .join('');
  }

  public create(): void {
    if (!this.state.busy && this.validPseudo) {
      this.storeService.set('last_connected_address', this.address);
      this.bootstrap.create(
        this.address.trim(),
        this.password,
        this.pseudo.trim(),
      );
    }
  }

  public join(): void {
    if (!this.state.busy && this.validCode && this.validPseudo) {
      this.storeService.set('last_connected_address', this.address);
      this.bootstrap.join(
        this.address.trim(),
        this.password,
        this.code,
        this.pseudo.trim(),
      );
    }
  }

  public errorKey(): string {
    return `CAMPAIGN_BOOTSTRAP.ERROR.${this.state.error || 'UNKNOWN'}`;
  }

  @HostListener('document:keydown.escape', ['$event'])
  public onEscape(event: KeyboardEvent): void {
    if (
      this.visible &&
      !this.state.busy &&
      (this.state.screen === 'create' ||
        this.state.screen === 'join' ||
        this.state.screen === 'error')
    ) {
      event.preventDefault();
      this.bootstrap.back();
    }
  }
}
