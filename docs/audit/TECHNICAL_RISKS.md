# Registre des risques techniques

> **Statut : Audit + proposition de suivi**

| ID | Risque | Probabilité | Impact | Mesure recommandée |
|---|---|---:|---:|---|
| R-01 | Le bridge de preview mono-client bloque plusieurs consommateurs | Élevée | Élevé | Lease manager avec owner token et priorité |
| R-02 | Réutilisation de `toggleDebugUI` comme canal de production | Élevée | Moyen | API CEF dédiée et typée |
| R-03 | Divergence importante avec upstream difficile à rebaser | Moyenne | Élevé | registre des patches, ADR, merges fréquents |
| R-04 | État trading uniquement en mémoire lors d’un crash serveur | Moyenne | Élevé | journal transactionnel ou politique d’abandon explicite |
| R-05 | Mutation d’inventaire distribuée non atomique au sens strict | Moyenne | Élevé | documenter le modèle saga + réconciliation, tests de panne |
| R-06 | Versionnement protocole absent pour futurs adapters dynamiques | Élevée | Élevé | enveloppe versionnée avant SDK tiers |
| R-07 | Papyrus/CK deviennent source de vérité implicite | Moyenne | Élevé | bridge intention → validation → événement canonique |
| R-08 | Skip Helgen laisse des quêtes vanilla dans un état incohérent | Élevée | Élevé | matrice de stages/globals et tests de reprise |
| R-09 | Personnages externes/cheatés rejoignent une campagne | Élevée | Élevé | binding campagne-personnage et bootstrap contrôlé |
| R-10 | Valen/scene suppose un seul `Game.GetPlayer()` | Élevée | Moyen | aliases locaux + coordination STRE |
| R-11 | Saturation de l’auberge à 10 joueurs | Moyenne | Moyen | marqueurs explicites, navmesh, tests de circulation |
| R-12 | Licences d’assets/voix insuffisamment explicites | Moyenne | Élevé | contributor agreement et fiche de provenance |
| R-13 | Documentation française limite les contributeurs internationaux | Moyenne | Moyen | anglais canonique public, français de travail ou traduction |
| R-14 | Export source exclut des dossiers source nommés Debug | Constaté | Faible | corriger le filtre du script |

## Décisions de réduction prioritaires

- Le trading doit être décrit comme une **saga compensée**, pas comme une transaction ACID distribuée.
- Le SDK de mods doit commencer par des adapters first-party compilés pour éviter de figer trop tôt une ABI.
- Les états de campagne doivent être sérialisables dès le premier MVP, même si la persistance disque vient plus tard.
- La preview doit devenir une ressource arbitrée avant d’être annoncée comme API tierce.
