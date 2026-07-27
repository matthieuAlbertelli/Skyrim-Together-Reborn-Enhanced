# Observabilité et journalisation

> **Statut : Patterns partiellement implémentés ; cible structurée à poursuivre**

## Logs actuels utiles

Character Build journalise notamment :

```text
[STRE][CharacterBuild][Server] Build accepted ... revision=... inventoryHash=... spellHash=...
[STRE][CharacterCreation] Spell grant applied form=...
[STRE][CharacterBuild][Client] Canonical spells applied ... count=... spellHash=...
[STRE][CharacterBuild][Client] Applied acknowledgement sent ...
[STRE][CharacterBuild][Server] Build applied ... level=1
```

Trading possède ses propres IDs de session, révision, apply et reconcile. La preview journalise session native, région et fitting.

## Format commun cible

Chaque transition critique doit inclure selon le subsystem :

- subsystem ;
- session/campaign/build ID ;
- player/server ID ;
- class/capability ;
- revision/version ;
- request/apply/reconcile ID ;
- inventory/spell hash lorsque pertinent ;
- plugin/FormID local pour les résolutions ;
- résultat ou code de rejet ;
- durée.

Exemple futur :

```text
[adapter=stre.alternate-start][campaign=42][capability=group.ready-check]
command_accepted request=918 player=7 version=12->13 ready=true
```

## Événements minimum

Implémentés ou partiels :

- trade state/apply/reconcile ;
- Character Build request/accepted/applied/rejected ;
- nettoyage et application d’inventaire/sorts ;
- résolution de plugin/FormID ;
- événements de magie distante ;
- preview session/fitting.

Futurs :

- adapter registration/compatibility ;
- snapshot persistant créé/appliqué/rejeté ;
- campaign transition ;
- player binding ;
- disconnect/reconnect ;
- CK bridge callback générique ;
- preview lease acquire/preempt/release.

## Niveaux

- `info` : transitions normales ;
- `warn` : retransmission, état stale, fallback, incompatibilité récupérable ;
- `error` : invariant brisé, snapshot invalide, application impossible ;
- `debug/trace` : détails raster, inventory et rendering lourds.

Les logs D3D et signatures de créatures doivent pouvoir être filtrés pour ne pas masquer les lignes Character Build/MagicService.

## Bundle de support

Exporter : versions, BuildVersion, SHA de l’ESP, load order, dernier log client de chaque joueur, log serveur, choix de classe/kits, hashes, runtime Skyrim et étapes de reproduction. Les données narratives secrètes futures doivent être anonymisées/filtrées.
