# Stratégie upstream

> **Statut : Politique active ; baseline encore à préciser en SHA complet pour la prochaine release**

## Constat

Le dépôt est un fork de `tiltedphoques/TiltedEvolution`. `UPSTREAM.md` enregistre actuellement :

- branche upstream `dev` ;
- base courte `ca3f3234` issue de l’audit du 19 juillet 2026 ;
- head STRE audité `a9f55908` ;
- version déclarée `0.1.0-alpha.1`.

Le placeholder historique a été supprimé, mais une release reproductible doit encore enregistrer les SHA complets et le head exact incluant les changements Unreleased.

## Règles

- enregistrer le commit base exact à chaque release ;
- merge/rebase upstream à cadence régulière ;
- isoler les modifications STRE dans des services et dossiers dédiés ;
- éviter les changements de style massifs sur fichiers upstream ;
- maintenir un registre des conflits récurrents ;
- ajouter tests de non-régression avant chaque intégration ;
- tester explicitement les zones hook-sensitive : UI native, preview, magie distante et `Script::CompileAndRun`.

## Classification des patches

- `isolated` : nouveau fichier/service ;
- `factory-registration` : opcode ou service registry ;
- `hook-sensitive` : reverse engineering/menu/native/magie ;
- `ui-invasive` : composants Angular partagés ;
- `protocol-breaking` : factories/opcodes/BuildVersion ;
- `ck-catalog-coupled` : ESP, FormIDs locaux, catalogue et UI doivent évoluer ensemble ;
- `build/release` : CI et packaging.

Les patches `hook-sensitive`, `protocol-breaking` et `ck-catalog-coupled` exigent une revue dédiée lors d’un update upstream.
