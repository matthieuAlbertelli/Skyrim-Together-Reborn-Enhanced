# Observabilité et journalisation

> **Statut : politique transversale active; couverture à compléter par feature**

Les marqueurs précis d’une feature peuvent vivre dans son `TEST_PLAN.md`. Ce document définit ce qu’un log STRE doit permettre de diagnostiquer globalement.

## Identifiants corrélables

Une transition critique doit inclure, selon le sous-système :

- subsystem;
- player/server ID;
- session/build/WorldEntity ID;
- request/apply/reconcile ID lorsque présent;
- revision/version;
- `GameId` / `PlacedReferenceId` lorsqu’ils sont nécessaires à l’identité;
- résultat ou code de rejet;
- fallback/timeout explicite;
- durée lorsque pertinente.

## Sous-systèmes actuels

### World Sync

Les logs doivent permettre de corréler :

```text
client authority
↔ server
↔ observer client
```

sur :

- création/adoption;
- binding/materialization;
- manipulation authority;
- hide/release;
- settlement;
- reconciliation;
- ownership/theft;
- forced release;
- timeout/disconnect.

### Trading

Les IDs de session, revision, apply et reconcile doivent permettre de suivre une saga complète et son recovery.

### Character Build

Les logs doivent permettre de rapprocher :

- logical selections;
- BuildVersion;
- inventory/spell hashes;
- accepted/applied/rejected;
- résolution de formulaire;
- application locale.

### Item Preview

Les logs détaillés de rendering/raster doivent rester filtrables et ne pas masquer les transitions fonctionnelles.

## Niveaux

- `info` — transition normale importante;
- `warn` — fallback, timeout, stale state, incompatibilité récupérable;
- `error` — invariant brisé, application impossible, snapshot incohérent;
- `debug/trace` — détails haute fréquence/diagnostic ponctuel.

## Logs temporaires

Un diagnostic très verbeux ajouté pour isoler un crash doit être :

- retiré après validation;
- ou converti en log stable de niveau adapté;
- ou conservé uniquement derrière un niveau debug/trace.

Les marqueurs de hotfix ne doivent pas devenir une API documentaire permanente.

## Support bundle

Pour une reproduction multijoueur, conserver au minimum :

- SHA STRE;
- runtime Skyrim;
- configuration serveur;
- versions/plugins SKSE pertinents;
- load order lorsque pertinent;
- logs client de chaque joueur;
- log serveur;
- étapes et heure du test.

Les données secrètes futures de campagne doivent être filtrées des bundles standard.
