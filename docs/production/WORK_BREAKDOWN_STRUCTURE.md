# Work Breakdown Structure

> **Statut : découpage structurel canonique; ne contient pas l’avancement**

Le WBS décrit **les domaines stables qui composent STRE**. La direction produit
et les release gates appartiennent à [`ROADMAP.md`](../../ROADMAP.md), l’état
implémenté/validé à [`docs/project/STATUS.md`](../project/STATUS.md), et
l’avancement opérationnel au GitHub Project défini par
[`GITHUB_GOVERNANCE.md`](GITHUB_GOVERNANCE.md).

## Carte des domaines produit

| Domaine | Responsabilité | Frontières principales du dépôt | Labels GitHub |
|---|---|---|---|
| Core multijoueur | transport, protocole, sessions, party, identité réseau, services client/serveur | `Code/client`, `Code/server`, `Code/encoding`, `Code/common` | `area: networking` |
| World/entity synchronization | cycle de vie des entités, acteurs, objets lâchés, références placées, grab, mounts, transforms et snapshots | services `Character`/`Object`/`Inventory`, messages WorldEntity, `docs/features/world-sync` | `area: world-sync`, `area: actors`, `area: mounts` |
| Trading | session d’échange, autorité, application, réconciliation et UX | domaine `Code/common/Trade`, services client/serveur, UI trade, `docs/features/trading` | `area: trading`, `area: ui` |
| Systèmes coopératifs | combat/magie/party/quests partagés et futures mécaniques coopératives acceptées | services existants et répertoire canonique de chaque feature | label du domaine concret; pas de label générique artificiel |
| Alternate Start et création | nouveau jeu/Helgen, RaceMenu, création, reset, build canonique, solo et départ | Character Creation, Alternate Start UI, CK/Papyrus, `docs/features/alternate-start` | `area: alternate-start`, `area: ui` |
| Programme des classes | roster canonique, kits, capacités/perks, quêtes personnelles et validation des 21 classes | roster/spec, catalogues partagés, UI, CK/Papyrus, issues de classes | `area: classes`, `area: quests` |
| Campagne coopérative | identité de campagne/personnage, persistance, snapshots, phases, roster, readiness, reconnect et late join | architecture Campaign State et futures implémentations client/serveur | `area: campaign`, `area: networking` |
| Valen et narration | contrat narratif, acteur, voix, scène collective et projection de phase | `docs/narrative`, `docs/art`, `docs/audio`, CK/Papyrus | `area: valen`, `area: quests` |
| Headquarters et housing | hub, dix chambres, identité des chambres, ownership/assignment et restauration | contenu CK, campagne/persistance, `docs/features/alternate-start` | `area: housing` |
| UI et preview | surfaces Angular/CEF, commandes typées, preview 3D et accessibilité | `Code/skyrim_ui`, services UI/preview, `docs/features/item-preview` | `area: ui`; le consommateur ajoute son domaine |
| Build, test et release | toolchain, CI, packaging, compatibilité, preuves et publication | xmake, `.github/workflows`, `Tools`, `docs/development`, `docs/testing` | `area: build-ci` |
| Documentation et gouvernance | sources de vérité, ADR, contribution, GitHub Project/Milestone et historique | `docs`, fichiers de gouvernance racine, `.github` | `area: docs` |

L’**ownership et l’autorité** sont une règle transversale, pas un domaine séparé:
chaque état partagé doit nommer son autorité dans l’architecture et dans l’issue
qui le modifie. Le **CK/Papyrus** est une frontière d’adaptation et de contenu,
pas une seconde source de vérité; il projette le domaine auquel le record ou le
script appartient.

Les labels actuels couvrent les domaines actionnables. `area: campaign` est
nécessaire car la continuité/persistance/phases constitue un produit durable et
ne se réduit ni au transport réseau ni à Alternate Start. Des labels séparés
`core`, `authority`, `CK`, `Papyrus`, `testing`, `release` ou `co-op` ne sont pas
créés: ce sont des couches ou préoccupations transversales déjà représentées par
le domaine concret, `area: build-ci` ou `area: docs`.

## 1. Core multijoueur et networking

- 1.1 Client runtime et services
- 1.2 Server runtime et services
- 1.3 Transport, protocol factories et bornes
- 1.4 Identités partagées et contrats d’autorité
- 1.5 Session, party, joueur et authentification
- 1.6 Observabilité et diagnostic
- 1.7 Politique des plugins natifs

## 2. World/entity synchronization

- 2.1 Actor/entity lifecycle
- 2.2 Dynamic dropped-item materialization
- 2.3 Placed-reference lazy adoption
- 2.4 Grab/manipulation authority et Better Grabbing
- 2.5 Local Havok settlement/reconciliation
- 2.6 Ownership/provenance et interaction
- 2.7 Mount occupancy et présentation
- 2.8 Snapshot/late join
- 2.9 Durable world persistence/checkpoints
- 2.10 Validation des références scriptées/quest et nouveaux types

## 3. Trading

- 3.1 Domaine/session et protocole
- 3.2 Autorité serveur et validation
- 3.3 Application client, idempotence et réconciliation
- 3.4 UI/UX et preview
- 3.5 Métadonnées d’instance
- 3.6 Gold/stack support
- 3.7 Reconnect, recovery et validation

## 4. Systèmes coopératifs et Item Preview

- 4.1 Combat, magie, party et quest synchronization existants
- 4.2 Downed/recovery et autres features uniquement après acceptation produit
- 4.3 Item Preview native/controller/solver/host
- 4.4 Lease/arbitration multi-consommateurs
- 4.5 Lifecycle/concurrency tests

## 5. Alternate Start et Character Creation

- 5.1 CK cell/quest/aliases
- 5.2 New-game interception et Helgen continuity
- 5.3 RaceMenu, Angular flow et preview
- 5.4 Anti-import/reset
- 5.5 CharacterBuild catalog, inventaire et sorts
- 5.6 Solo fallback
- 5.7 Valen/ready/departure projection
- 5.8 Save/load/reconnect recovery

## 6. Programme des 21 classes

- 6.1 Roster et identité canonique
- 6.2 Kits/loadouts exacts
- 6.3 Capacités/perks coopératifs
- 6.4 Une quête personnelle par classe
- 6.5 Catalogues, CK/Papyrus, UI et versioning cohérents
- 6.6 Solo/server equivalence
- 6.7 Validation automatisée et en jeu

## 7. Campagne coopérative

- 7.1 Campaign/character identity et binding
- 7.2 Stockage, migration et snapshot
- 7.3 Roster/readiness
- 7.4 Introduction, départ et phases partagées
- 7.5 Session Manager/Dragonborn policy
- 7.6 Reconnect/late join
- 7.7 Journal/outbox et recovery
- 7.8 Projection locale CK/UI

## 8. Valen, headquarters et housing

- 8.1 Narrative/scene contract
- 8.2 Actor, art, voice et localisation
- 8.3 CK scene et phase projection
- 8.4 Headquarters layout/navmesh/performance
- 8.5 Dix identités de chambre utilisables
- 8.6 Assignment/ownership/persistence/restoration

## 9. CK/Papyrus, UI et intégration de mods

- 9.1 Bridge CK/Papyrus ↔ STRE
- 9.2 Projection idempotente et fonctionnement solo
- 9.3 Angular/CEF surfaces et contrats typés
- 9.4 First-party adapters
- 9.5 Capability/version negotiation
- 9.6 SDK tiers seulement après validation first-party suffisante

## 10. Opérations projet

- 10.1 Build/CI/package
- 10.2 Tests, compatibilité et evidence bundles
- 10.3 Versioning, changelog, tags et GitHub Releases
- 10.4 Documentation, ADR et historique
- 10.5 GitHub governance et contribution
- 10.6 Licences/provenance et sécurité
