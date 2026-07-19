# ADR-0008 — La preview native devient une ressource à leases

- **Statut : Proposed**
- **Date : 2026-07-19**

## Contexte

Le bridge actuel n’accepte qu’un client et reste nominalement lié au trading.

## Décision

Introduire un `ItemPreviewRuntime` qui attribue une lease à un owner, arbitre les priorités et encapsule entièrement le host menu et la session native.

## Conséquences

- réutilisation réelle par plusieurs features ;
- préemption et ownership testables ;
- migration de `TradeItemPreviewService` ;
- API externe possible sans exposer les pointeurs natifs.
