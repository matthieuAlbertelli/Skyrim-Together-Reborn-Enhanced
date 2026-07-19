# Audit de l’état actuel

> **Statut : Implémenté / constaté dans l’archive**  
> **Archive auditée :** `Skyrim_Together_Reborn_Enhanced_source_audit_20260719_233710.zip`  
> **Version déclarée :** `0.1.0-alpha.1`

## Résumé exécutif

Le dépôt est un fork fonctionnel de Skyrim Together Reborn dont la première verticale majeure est un système de trading joueur-à-joueur. Le code dépasse nettement la documentation actuelle : il comporte un domaine métier indépendant, un service serveur autoritaire, un protocole dédié, des journaux d’idempotence client, une réconciliation après état incertain, une UI Angular et une preview 3D native modulaire.

La base est prometteuse pour le futur Mod Integration Framework, car elle montre déjà une séparation entre :

- domaine pur ;
- protocole ;
- autorité serveur ;
- adaptation client/Skyrim ;
- UI ;
- composant technique réutilisable de preview.

## Métriques de la verticale Trading/Preview

| Sous-système | Fichiers | Lignes approximatives |
|---|---:|---:|
| Domaine `Code/common/Trade` | 10 | 1 368 |
| Service serveur | 2 | 1 969 |
| Services client trading/menu/preview | 6 | 2 084 |
| Cœur Item Preview + host menu | 15 | 3 090 |
| Protocole et structs | 32 | 1 082 |
| Tests trading | 5 | 1 240 |
| UI Angular liée au trading | 6 | 1 856 |

Le dépôt contient **44 cas de test** trading et **13 messages réseau dédiés** : 7 requêtes client et 6 notifications serveur.

## Architecture actuelle du trading

### Domaine pur

`Code/common/Trade/` contient :

- `Session` : automate de négociation ;
- `BuildMutationPlan` : validation et calcul des deltas ;
- `Application` : suivi des résultats d’application par participant ;
- `BuildReconciliationPlan` et `Reconciliation` : retour vers des quantités absolues ;
- types, erreurs et structures indépendants de Skyrim et du transport.

### Serveur autoritaire

`Code/server/Services/TradeService.cpp` :

- alloue les sessions ;
- empêche un joueur d’être dans plusieurs trades ;
- accepte/rejette les invitations ;
- valide les révisions ;
- construit des snapshots d’inventaire ;
- génère les plans de mutation ;
- suit les résultats clients ;
- commit les mutations côté état serveur ;
- déclenche une réconciliation en cas d’incertitude ;
- gère expiration, déconnexion et nettoyage.

Constantes observées : invitation 30 s, sweep 1 s, application 15 s, réconciliation 30 s, rétention terminale 5 s.

### Client

`Code/client/Services/Generic/TradeService.cpp` :

- envoie les intentions ;
- conserve l’état de session reçu ;
- applique les plans de mutation ;
- journalise les résultats par `ApplyId` et `ReconcileId` ;
- répond de façon idempotente aux retransmissions.

`TradeMenuService` convertit l’état natif en JSON et publie `tradeState` vers CEF.

### UI

L’UI Angular possède :

- états invitation, outgoing, session et erreur ;
- catégories d’inventaire ;
- quantité offerte ;
- confirmations ;
- preview d’objets ;
- observation de la région de preview avec `ResizeObserver`.

### Preview 3D

Le refactor a extrait :

- `ItemPreviewController` : orchestration et état du fitting ;
- `ItemPreviewNativeSession` : cycle de vie `Inventory3DManager` ;
- `ItemPreviewHostSession` : ouverture/fermeture idempotente du host menu ;
- `ItemPreviewHostBridge` : pont singleton vers un client ;
- `ItemPreviewFitSolver` : calcul de transformation ;
- `ItemPreviewRasterMeasurer` : mesure D3D11 ;
- `TradePreviewHostMenu` : menu Scaleform non visible servant de host natif.

## Forces

1. **Domaine testable et indépendant.**
2. **Révisions et snapshots complets**, qui rendent les retransmissions sûres.
3. **Validation côté serveur** avant verrouillage et application.
4. **Réconciliation absolue**, plus robuste que de simples deltas rejoués.
5. **Collections bornées** à la désérialisation (`MaxLines`, `MaxMutations`, `MaxTargets`).
6. **Tests de protocole round-trip** et cas de données surdimensionnées.
7. **Refactor de preview réellement séparé du domaine trading.**
8. **Arbitrage de surface CEF** via `UiSurfaceService`.

## Limites et dette technique

### API UI provisoire

Le frontend appelle `toggleDebugUI('__trade__', ...)`. Le code précise que l’ancienne commande debug est détournée. Cette interface ne doit pas devenir le contrat public d’un SDK.

### Preview encore mono-consommateur

`ItemPreviewHostBridge` conserve un seul pointeur `ItemPreviewHostClient`. Un deuxième consommateur ne peut pas se binder. Cela prépare la réutilisation interne, mais pas encore une plateforme multi-mods.

### Couplage nominal au trading

Le host menu s’appelle `TradePreviewHostMenu` et de nombreux logs commencent par `Trade preview`. La mécanique interne est générique, mais ses frontières publiques restent spécifiques.

### Pas d’API externe stable

Aucun manifeste de capability, ABI de plugin, binding Papyrus ou protocole générique de preview n’est présent. La « brique d’API » est à ce stade une **API C++ interne au client STRE**.

### Persistance serveur limitée

Les maps de sessions/applications sont en mémoire. Le code audité ne montre pas de persistance de transactions à travers un redémarrage serveur.

### Reconnexion pendant une transaction

La déconnexion est gérée, mais il n’existe pas encore de snapshot de trade restaurable après reconnexion au sens d’une campagne persistante.

### Documentation historique insuffisante au moment de l’audit

L’archive auditée contenait des résumés trop courts (`docs/features/trading.md`, `docs/architecture/preview-system.md`) et un placeholder dans `UPSTREAM.md`. La consolidation documentaire corrige ces points, sans modifier le constat sur l’état du code au moment de l’audit.

### Export d’audit incomplet sur les dossiers nommés Debug

Le script source-only a exclu tout segment appelé `Debug`, ce qui a également retiré des fichiers source comme `Code/client/Services/Debug/TradeDebugService.cpp`. Le cœur fonctionnel est présent, mais le prochain export doit limiter l’exclusion aux configurations de build, pas aux répertoires source nommés `Debug`.

## Alternate Start

Aucun code spécifique à Alternate Start, Valen, Campaign State ou Mod Adapter n’a été trouvé dans l’archive. Le plugin CK est donc un chantier séparé qui devra être ajouté ou versionné dans un dépôt associé.

## Recommandations immédiates

1. Corriger `UPSTREAM.md` et les métadonnées héritées.
2. Publier une architecture Trading complète.
3. Remplacer le bridge `toggleDebugUI` par un canal typé dédié.
4. Faire évoluer la preview vers un gestionnaire de leases.
5. Ajouter des tests du solver et des tests d’intégration réseau.
6. Versionner le plugin Alternate Start et ses sources Papyrus dans une structure traçable.
7. Utiliser Alternate Start comme premier adaptateur first-party avant d’ouvrir un SDK tiers.
