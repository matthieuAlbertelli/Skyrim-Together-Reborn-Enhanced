# État courant de STRE

> **Statut : source de vérité de l’état implémenté/validé**
> **Dernière mise à jour : 12 août 2026**

Ce document décrit **où en est réellement le dépôt**. La direction produit et les release gates appartiennent à [`ROADMAP.md`](../../ROADMAP.md), l’avancement opérationnel au GitHub Project défini par [`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md), et les détails techniques aux documents de chaque feature.

## World Sync

### Implémenté et validé en jeu

- les objets lâchés reçoivent une identité réseau stable `WorldEntityId`;
- chaque client conserve sa simulation Havok locale;
- le joueur à l’origine de l’action conserve l’autorité de settlement jusqu’au transform final;
- les clients distants ne sont corrigés qu’en cas de divergence significative;
- les snapshots permettent la matérialisation/liaison tardive des WorldEntities;
- les références mobiles déjà présentes dans le monde sont adoptées paresseusement via leur `PlacedReferenceId`;
- une référence placée est liée à la référence Skyrim locale existante, jamais dupliquée;
- Better Grabbing est requis par défaut en multijoueur via la politique générique des plugins SKSE natifs;
- pendant un grab distant, l’objet est masqué chez les observateurs plutôt que streamé en continu;
- au release, les références placées utilisent le chemin `MoveTo` existant de STR sur la game thread via `RunnerService`;
- l’ownership/provenance est transporté dans les chemins supportés;
- grabber un objet possédé sans être owner déclenche le système de vol vanilla;
- l’ouverture du `Dialogue Menu` force proprement la fin d’un grab pour éviter de bloquer les dialogues de garde/arrestation.

### Limites connues

- les noms personnalisés reposant sur `ExtraTextDisplayData` ne sont pas encore synchronisés;
- les références scriptées/objets de quête nécessitent encore une campagne de validation dédiée;
- la persistance durable du monde après redémarrage serveur/save branches n’est pas encore implémentée;
- le modèle WorldEntity n’est pas encore généralisé à tous les types de références monde.

Voir [`docs/features/world-sync/`](../features/world-sync/).

## Trading

### Implémenté

- domaine de session dédié;
- protocole serveur autoritaire;
- offres révisionnées;
- plans de mutation déterministes;
- application client idempotente;
- réconciliation vers des quantités absolues;
- UI Angular/CEF;
- preview 3D native.

### Limites

- pas encore de piles divisibles ni d’or;
- reconnect d’un trade actif à renforcer;
- le protocole MVP ne transporte pas toutes les métadonnées d’instance;
- les objets portant un ownership non représentable sont rejetés plutôt que transférés en perdant leurs données.

Voir [`docs/features/trading/`](../features/trading/).

## Item Preview

### Implémenté

- session native;
- contrôleur;
- host bridge/session;
- solver de cadrage;
- mesure raster;
- consommateurs Trading et Character Creation.

### Limite structurante

Le bridge reste mono-consommateur actif. Un système de leases/ownership explicite reste nécessaire avant de parler d’API tierce stable.

Voir [`docs/features/item-preview/`](../features/item-preview/).

## Alternate Start / Character Build

### Implémenté et smoke-testé

- plugin `STRE_AlternateStart.esp` versionné avec PSC/PEX;
- auberge, quête, aliases et sièges;
- RaceMenu + Character Creation Angular;
- catalogue partagé Warrior/Mage/Thief;
- inventaire et sorts canoniques;
- hashes et accusé d’application;
- fallback local sans serveur;
- Destruction/Altération Mage;
- buffs coopératifs ciblés testés entre deux PC.

Le catalogue courant utilise `BuildVersion = 5`.

### Limites

- interception complète du nouveau jeu/skip Helgen à terminer;
- Valen et départ narratif non finalisés;
- persistance/reconnexion des builds non durable;
- plusieurs écoles/kits restent à matérialiser;
- reset des compétences/perks/historique d’attributs encore incomplet.

Voir [`docs/features/alternate-start/`](../features/alternate-start/).

## Régressions importantes corrigées

- régression de nage apparue pendant les travaux STRE;
- crash observateur lors du repositionnement d’une référence placée;
- état de grab bloqué pendant les dialogues de garde/arrestation;
- dépendance Google Fonts bloquant les builds Angular hors ligne.

## Règle de communication

Ne pas déduire l’état du projet depuis un ancien rapport de jalon ou un audit daté.

- **État courant :** ce document.
- **Direction produit et release gates :** [`ROADMAP.md`](../../ROADMAP.md).
- **Avancement opérationnel :** GitHub Project, selon [`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md).
- **Historique :** [`CHANGELOG.md`](../../CHANGELOG.md) et `docs/audit/`.
