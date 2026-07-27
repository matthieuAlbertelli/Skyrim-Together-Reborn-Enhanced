# Vue d’ensemble du système

> **Statut : Implémenté pour Trading et Character Build / Proposé pour Campaign State et SDK générique**

## Couches

```text
┌──────────────────────────────────────────────────────────────┐
│ Contenu Skyrim                                               │
│ Cellules, quêtes, Papyrus, objets, sorts, animations         │
└───────────────────────────────┬──────────────────────────────┘
                                │ événements / application locale
┌───────────────────────────────▼──────────────────────────────┐
│ Bridges client                                               │
│ TES events · CEF/V8 bridge · Skyrim native adapters          │
└───────────────────────────────┬──────────────────────────────┘
                                │ intentions / résultats
┌───────────────────────────────▼──────────────────────────────┐
│ STRE Client                                                  │
│ Services · UI Surface · local appliers · verification        │
└───────────────────────────────┬──────────────────────────────┘
                                │ protocole versionné
┌───────────────────────────────▼──────────────────────────────┐
│ STRE Server                                                  │
│ Validation · canonical inventory/spells · state broadcasts   │
└──────────────────────────────────────────────────────────────┘
```

## Architecture actuelle observée

Les `World` client et serveur enregistrent leurs services dans le contexte EnTT. Le bus `entt::dispatcher` relie messages réseau, updates et événements de jeu. Les messages sont des types statiques enregistrés dans les factories de protocole.

### Trading

```text
Angular action
→ TradeMenuService / TradeService client
→ ClientMessage
→ TradeService serveur
→ domaine Trade
→ ServerMessage
→ application locale / UI
```

### Alternate Start / Character Build

```text
Stage de quête CK 20
→ CharacterCreationService
→ RaceMenu + Angular loadouts
→ sélection logique race/classe/kits
→ CharacterBuildRequest
→ CharacterBuildService serveur
→ inventaire + sorts canoniques + hashes
→ CharacterBuildResponse
→ nettoyage et application locale
→ CharacterBuildAppliedRequest
→ validation des hashes
→ NotifyCharacterBuildState(Applied)
```

En mode hors ligne, `CharacterCreationService` utilise le même catalogue partagé et applique le build localement sans connexion au serveur.

## Frontières de responsabilité actuelles

### Creation Kit / plugin Alternate Start

- cellule, quête, aliases, sièges et records de gameplay ;
- déclenchement local de Character Creation ;
- modèles, noms, enchantements, sorts et effets ;
- fallback solo au niveau du flux de création.

Le plugin n’est pas la source de vérité du build multijoueur.

### Catalogue partagé

- classes/options autorisées ;
- activation conditionnelle des groupes ;
- objets, quantités, équipement et sorts dérivés ;
- version de build.

### STRE Client

- UI et collecte des choix ;
- transport des intentions ;
- nettoyage anti-import ;
- application de l’inventaire et des sorts ;
- vérification locale et accusé d’application ;
- synchronisation des buffs distants reconnus.

### STRE Server

- validation de version, race, classe et options ;
- résolution plugin/FormID local ;
- construction des snapshots canoniques ;
- calcul des hashes ;
- état Pending/Applied pendant la session ;
- niveau serveur à 1 après accusé valide.

## Architecture cible

Le Mod Integration Runtime générique ajoutera :

```text
Mod solo
→ Adapter local
→ Intent versionnée
→ Capability Runtime
→ Canonical State persistant
→ Snapshot / Event
→ Adapter local
→ conséquence Skyrim
```

Campaign State, adapter registry, persistance et reconnexion ne sont pas encore fournis par le service Character Build actuel.

## Principes transverses

- Aucun état critique n’est défini uniquement par un menu, une scène ou un stage de quête.
- Le client n’envoie pas de liste arbitraire de FormIDs d’objets ou de sorts.
- Les snapshots canoniques sont validés avant application.
- Les accusés d’application portent des hashes déterministes.
- Les FormIDs chargés ne sont jamais codés en dur ; le code résout plugin + FormID local.
- Les APIs tierces restent proposées tant qu’elles n’ont pas été validées par plusieurs intégrations first-party.
