# Registre des risques techniques

> **Statut : source de vérité des risques techniques actifs**
> **Dernière mise à jour : 10 août 2026**

| ID | Risque | Impact | Mitigation courante |
|---|---|---:|---|
| R-01 | Divergence importante avec upstream | Élevé | patches isolés, ADR, intégrations régulières |
| R-02 | Mutation moteur depuis un mauvais thread | Élevé | marshalling via `RunnerService`/game update |
| R-03 | Wrapper reverse-engineered avec ABI supposée | Critique | préférer primitives STR validées; preuve de signature avant nouveau wrapper |
| R-04 | Havok distant combattu par du streaming de transforms | Élevé | ADR-0014 : Havok local + settlement ponctuel |
| R-05 | Référence placée dupliquée à l’adoption | Élevé | `PlacedReferenceId -> WorldEntityId` + binding local existant |
| R-06 | Objet scripté/quest affecté par hide/enable/reposition | Élevé | campagne de validation dédiée avant support garanti |
| R-07 | Métadonnées d’instance perdues pendant transfert | Élevé | `Inventory::Entry` enrichi + fail-closed quand un protocole ne sait pas préserver |
| R-08 | Nom personnalisé perdu | Moyen | `ExtraTextDisplayData` explicitement non supporté tant que crash-safe |
| R-09 | WorldEntity state perdu après restart/save branch | Élevé | future persistence/checkpoint versionnée |
| R-10 | Better Grabbing change son comportement/API interne | Moyen | dépendre d’événements/comportements Skyrim, pas de ses internals |
| R-11 | Plugin natif requis manquant/incompatible | Élevé | generic NativePlugins policy; version constraints à étudier |
| R-12 | Trading saga laisse un état incertain en panne | Élevé | idempotence + réconciliation absolue |
| R-13 | Preview mono-client bloque concurrence réelle | Élevé | futur lease manager |
| R-14 | Character Build perdu après reconnect/restart | Élevé | persistence/versioning avant Campaign State complet |
| R-15 | Skip Helgen laisse le vanilla incohérent | Élevé | matrice de stages/globals et tests de reprise |
| R-16 | Nettoyage anti-import incomplet | Élevé | politique skills/perks/attributes + tests avant/après |
| R-17 | Buffs distants reposent sur allowlist nominale | Moyen | capability/classification extensible avant expansion |
| R-18 | Documentation se désynchronise par duplication | Élevé | source-of-truth matrix + feature-local docs + history archive |

## Règle

Un risque résolu n’est pas maintenu indéfiniment ici comme état actif. Sa résolution appartient au changelog, à l’ADR ou à l’historique Git selon le cas.
