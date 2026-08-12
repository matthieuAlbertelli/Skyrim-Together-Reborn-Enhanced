# Registre des risques techniques

> **Statut : Snapshot historique au 27 juillet 2026 — non canonique pour les
> risques actifs.** Voir [`docs/production/RISK_REGISTER.md`](../../production/RISK_REGISTER.md).

| ID | Risque | Probabilité | Impact | État / mesure recommandée |
|---|---|---:|---:|---|
| R-01 | Le bridge de preview mono-client bloque un vrai usage concurrent | Élevée | Élevé | Second consommateur first-party validé, mais lease manager toujours requis |
| R-02 | Réutilisation de `toggleDebugUI` comme canal de production | Élevée | Moyen | API CEF dédiée et typée |
| R-03 | Divergence importante avec upstream difficile à rebaser | Moyenne | Élevé | registre des patches, ADR, merges fréquents |
| R-04 | État trading uniquement en mémoire lors d’un crash serveur | Moyenne | Élevé | journal transactionnel ou politique d’abandon explicite |
| R-05 | Mutation d’inventaire distribuée non atomique au sens strict | Moyenne | Élevé | documenter le modèle saga + réconciliation, tests de panne |
| R-06 | Versionnement protocole absent pour futurs adapters dynamiques | Élevée | Élevé | enveloppe versionnée avant SDK tiers |
| R-07 | Papyrus/CK deviennent source de vérité implicite | Moyenne | Élevé | conserver l’intention locale et l’état canonique serveur |
| R-08 | Skip Helgen laisse des quêtes vanilla dans un état incohérent | Élevée | Élevé | matrice de stages/globals et tests de reprise |
| R-09 | Personnages externes/cheatés rejoignent une campagne | Moyenne | Élevé | nettoyage + build canonique implémentés ; binding persistant encore requis |
| R-10 | Valen/scène suppose un seul `Game.GetPlayer()` | Élevée | Moyen | aliases locaux + coordination STRE |
| R-11 | Saturation de l’auberge à 10 joueurs | Moyenne | Moyen | marqueurs explicites, navmesh, tests de circulation |
| R-12 | Licences d’assets/voix insuffisamment explicites | Moyenne | Élevé | contributor agreement et fiche de provenance |
| R-13 | Documentation française limite les contributeurs internationaux | Moyenne | Moyen | anglais canonique public, français de travail ou traduction |
| R-14 | Export source exclut des dossiers source nommés Debug | Constaté | Faible | corriger le filtre du script |
| R-15 | Build de personnage perdu après reconnect ou redémarrage serveur | Élevée | Élevé | persistance versionnée + restauration idempotente |
| R-16 | Les buffs distants reposent sur une allowlist nominale de FormID locaux | Moyenne | Moyen | classification par keyword/capability avant extension à de nombreux mods |
| R-17 | Nettoyage incomplet des compétences/perks/statistiques historiques | Élevée | Élevé | jalon de reset contrôlé, tests avant/après et politique explicite |
| R-18 | `Script::CompileAndRun` dépend d’un Address Library ID runtime | Moyenne | Élevé | conserver validation 1.6.1170, fallback documenté et logs explicites |

## Décisions de réduction prioritaires

- Le trading est une **saga compensée**, pas une transaction ACID distribuée.
- Le build canonique réduit le risque d’import cheaté, sans encore constituer un reset exhaustif du personnage.
- La persistance des builds doit précéder le Campaign State complet.
- Le SDK de mods doit commencer par des intégrations first-party compilées pour éviter de figer trop tôt une ABI.
- La preview doit devenir une ressource arbitrée avant d’être annoncée comme API tierce.
- La classification des sorts coopératifs doit devenir extensible avant l’ajout massif de sorts custom.
