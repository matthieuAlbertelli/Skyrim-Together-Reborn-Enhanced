# Vue d’ensemble du système

> **Statut : Implémenté pour Trading / Proposé pour Campaign & Mod Integration**

## Couches

```text
┌──────────────────────────────────────────────────────────────┐
│ Contenu Skyrim                                               │
│ Cellules, PNJ, quêtes, scènes, Papyrus, objets, animations   │
└───────────────────────────────┬──────────────────────────────┘
                                │ intentions / effets locaux
┌───────────────────────────────▼──────────────────────────────┐
│ Bridges client                                               │
│ CK/Papyrus Bridge · CEF Bridge · Skyrim native adapters      │
└───────────────────────────────┬──────────────────────────────┘
                                │ commands / events
┌───────────────────────────────▼──────────────────────────────┐
│ STRE Client                                                  │
│ Services · Mod Adapter Runtime · UI Surface · Local appliers │
└───────────────────────────────┬──────────────────────────────┘
                                │ protocole versionné
┌───────────────────────────────▼──────────────────────────────┐
│ STRE Server / Campaign Authority                             │
│ Validation · Canonical State · Snapshots · Persistence       │
└──────────────────────────────────────────────────────────────┘
```

## Architecture actuelle observée

Les `World` client et serveur enregistrent leurs services dans le contexte EnTT. Le bus `entt::dispatcher` relie messages réseau, updates et événements de jeu. Les messages sont des types statiques enregistrés dans les factories de protocole.

Le trading illustre le chemin complet :

```text
Angular action
→ OverlayClient
→ TradeMenuService / TradeService client
→ ClientMessage
→ TradeService serveur
→ domaine Trade
→ ServerMessage
→ application locale / UI
```

## Architecture cible

Le Mod Integration Runtime ajoute un niveau générique :

```text
Mod solo
→ Adapter local
→ Intent
→ Capability Runtime
→ Command autoritaire
→ Canonical State
→ Event
→ Adapter local
→ conséquence Skyrim
```

## Frontières de responsabilité

### Creation Kit / mod solo

- contenu, présentation et progression locale ;
- lecture/écriture d’éléments Skyrim ;
- fallback solo ;
- application des conséquences validées.

### Mod Adapter

- traduction entre concepts du mod et concepts STRE ;
- déclaration des capabilities ;
- observation des signaux ;
- validation locale de forme ;
- capture/application de snapshot ;
- gestion des erreurs de compatibilité.

### STRE Client

- transport d’intentions ;
- cache local de l’état canonique ;
- bridge UI et Skyrim ;
- journaux idempotents ;
- application ordonnée des événements.

### STRE Server

- autorité ;
- invariants ;
- versionnement ;
- conflits ;
- diffusion ;
- persistance ;
- reconnexion.

## Principes transverses

- Aucun état critique n’est défini uniquement par l’affichage d’un menu ou d’une scène.
- Toute commande possède un identifiant ou une version permettant de détecter les doublons.
- Les snapshots sont complets ; les événements sont incrémentaux.
- Les adapters ne doivent pas appeler directement des services internes non contractuels.
- Les APIs externes sont versionnées et bornées en taille.
