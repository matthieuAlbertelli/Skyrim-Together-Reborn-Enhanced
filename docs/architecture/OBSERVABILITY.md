# Observabilité et journalisation

> **Statut : Proposition fondée sur les logs existants**

## Format commun

Chaque transition critique doit inclure :

- subsystem ;
- campaign/session id ;
- player id ;
- adapter/capability ;
- ancienne et nouvelle version ;
- ancienne et nouvelle phase ;
- request/apply/reconcile id ;
- résultat ou code de rejet ;
- durée lorsque pertinent.

Exemple :

```text
[adapter=stre.alternate-start][campaign=42][capability=group.ready-check]
command_accepted request=918 player=7 version=12->13 ready=true
```

## Événements minimum

- adapter registration/compatibility ;
- command requested/accepted/rejected ;
- snapshot created/applied/rejected ;
- event applied/duplicate/stale ;
- campaign transition ;
- player binding ;
- disconnect/reconnect ;
- CK bridge callback ;
- preview lease acquire/preempt/release ;
- trade state/apply/reconcile.

## Niveaux

- `info` : transitions normales ;
- `warn` : retransmission, état stale, fallback, incompatibilité récupérable ;
- `error` : invariant brisé, snapshot invalide, application impossible ;
- `debug/trace` : détails raster, inventory et rendering lourds.

Les logs de diagnostic D3D très verbeux observés dans le host menu doivent être contrôlables par catégorie pour éviter de saturer les fichiers en production.

## Bundle de support

Outil recommandé : exporter automatiquement versions, load order, adapters, derniers logs, état de campagne anonymisé et hash de configuration.
