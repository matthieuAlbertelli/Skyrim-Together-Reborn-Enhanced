# Trading — Product Spec

> **Status:** implemented alpha feature; consolidated specification.

## Problem

Skyrim Together Reborn does not provide a secure, understandable player-to-player
trade flow. Dropping objects on the ground is unreliable, not very immersive,
and provides no mutual-consent guarantee.

## User goal

Two nearby players can open a transaction, compose their offers, inspect items,
confirm the same revision, and receive a clear result: completed, cancelled, or
failed.

## Main flow

1. Player A activates Player B and requests a trade.
2. B accepts or rejects within 30 seconds.
3. Both players add or remove quantities.
4. Any modification clears previous confirmations.
5. Both players confirm the same revision.
6. The server validates inventories and locks the transaction.
7. Each client applies its local plan.
8. The server confirms or starts reconciliation.
9. The UI displays the terminal state.

## Current alpha rules

An item is transferable only if it:

- exists in a positive quantity;
- is not a quest item;
- is not equipped;
- has no charge, custom enchantment, poison, soul, or extra health/durability;
- is not ambiguous in the logical inventory;
- can be represented by the canonical mod/base ID pair.

Offers and plans are limited to 64 lines.

## UX states

- invitation received;
- outgoing invitation;
- synchronization;
- negotiation;
- locked;
- application;
- completed;
- cancelled;
- failed.

## Alpha non-goals

- gold trading;
- advanced splitting of stacks with distinct instances;
- unique enchanted items;
- persistence across server restart;
- multi-player transactions;
- marketplace.

## Acceptance criteria

- a stale offer cannot be confirmed;
- a player cannot trade with themselves or a busy player;
- retransmitting an identical command does not duplicate items;
- partial failure leads to reconciliation or an explicit state;
- closing and reopening the UI does not change server state;
- the 3D preview does not block input after closing.
