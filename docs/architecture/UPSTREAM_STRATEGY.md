# Stratégie upstream

> **Statut : politique active**

La baseline upstream courante est enregistrée **uniquement** dans [`UPSTREAM.md`](../../UPSTREAM.md). Ce document décrit la politique d’intégration et ne duplique pas les SHA/version courants.

## Règles

- enregistrer le commit upstream exact à chaque release;
- intégrer upstream régulièrement plutôt que par gros rattrapage;
- isoler les modifications STRE dans des services/dossiers dédiés lorsque possible;
- éviter les changements de style massifs dans les fichiers upstream;
- maintenir les divergences structurantes derrière des contrats documentés;
- tester avant/après chaque intégration les zones hook-sensitive et protocol-breaking.

## Classification des patches

- `isolated` — nouveau fichier/service avec faible conflit;
- `factory-registration` — opcode ou service registry;
- `hook-sensitive` — reverse engineering, menus, engine wrappers, magie, physics;
- `ui-invasive` — composants Angular/CEF partagés;
- `protocol-breaking` — factories, opcodes, schémas ou versions incompatibles;
- `ck-catalog-coupled` — ESP/records/catalogue/UI liés;
- `build/release` — CI, packaging, toolchain.

Les patches `hook-sensitive`, `protocol-breaking` et `ck-catalog-coupled` exigent une revue dédiée lors d’un update upstream.

## Garde-fous STRE

- ne pas réintroduire un wrapper moteur dont la signature ABI n’est pas démontrée;
- préférer les primitives STR déjà reverse-engineerées et validées;
- conserver les mutations moteur réseau sur le chemin game-thread;
- préserver les contrats d’identité/autorité STRE lors des merges;
- mettre à jour tests et ADR si upstream invalide une hypothèse structurante.
