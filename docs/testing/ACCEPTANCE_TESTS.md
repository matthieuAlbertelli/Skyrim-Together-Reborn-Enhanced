# Index des tests d’acceptation

> **Statut : index canonique; les scénarios détaillés vivent avec leur feature**

Ce fichier ne duplique pas les cas de test. Il indique où se trouvent les critères d’acceptation détaillés.

| Feature | Plan canonique |
|---|---|
| World Sync | [`../features/world-sync/TEST_PLAN.md`](../features/world-sync/TEST_PLAN.md) |
| Trading | [`../features/trading/TEST_PLAN.md`](../features/trading/TEST_PLAN.md) |
| Item Preview | [`../features/item-preview/TEST_PLAN.md`](../features/item-preview/TEST_PLAN.md) |
| Alternate Start / Character Build | [`../features/alternate-start/TEST_PLAN.md`](../features/alternate-start/TEST_PLAN.md) |
| Downed State | documenter le plan dans `../features/downed-state/` lors de l’implémentation |

## Tests transverses

Les règles communes (environnement, preuves, logs, compatibilité, reproductibilité) appartiennent à :

- [`TEST_STRATEGY.md`](TEST_STRATEGY.md)
- [`MULTIPLAYER_TEST_RUNBOOK.md`](MULTIPLAYER_TEST_RUNBOOK.md)
- [`COMPATIBILITY_MATRIX.md`](COMPATIBILITY_MATRIX.md)

Lorsqu’un nouveau scénario concerne une seule feature, l’ajouter à son `TEST_PLAN.md` plutôt qu’ici.
