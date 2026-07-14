export type TradeCategory =
  | 'all'
  | 'weapons'
  | 'armor'
  | 'consumables'
  | 'books'
  | 'misc';

export type TradePhase =
  | 'pending'
  | 'negotiating'
  | 'locked'
  | 'applying'
  | 'completed'
  | 'cancelled'
  | 'failed'
  | 'unknown';

export interface TradeItem {
  id: string;
  name: string;
  category: Exclude<TradeCategory, 'all'>;
  icon: string;
  typeLabel: string;
  quantity: number;
  offered: number;
}

export interface TradeInviteViewState {
  visible: true;
  mode: 'invite';
  sessionId: string;
  inviterId: number;
}

export interface TradeOutgoingViewState {
  visible: true;
  mode: 'outgoing';
  targetPlayerId: number;
}

export interface TradeSessionViewState {
  visible: true;
  mode: 'session';
  phase: TradePhase;
  sessionId: string;
  localPlayerId: number;
  remotePlayerId: number;
  localConfirmed: boolean;
  remoteConfirmed: boolean;
  mutable: boolean;
  error: string;
  inventory: TradeItem[];
  localOffer: TradeItem[];
  remoteOffer: TradeItem[];
}

export interface TradeSimpleViewState {
  visible: boolean;
  mode?: 'syncing' | 'error';
  error?: string;
}

export type TradeViewState =
  | TradeInviteViewState
  | TradeOutgoingViewState
  | TradeSessionViewState
  | TradeSimpleViewState;
