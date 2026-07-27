# Jalons de production

> **Statut : Progression mise à jour le 27 juillet 2026**

## M0 — Repository Ready — En cours

**Démo :** un contributeur compile et lance les tests depuis un environnement documenté.

**Acquis :** baseline upstream, documentation, build local xmake, fichiers CK versionnés.
**Restant :** clean-machine matrix, CI, commande de test canonique, versions exactes.

## M1 — Trading Stabilized — Partiellement implémenté

**Acquis :** domaine, protocole, service serveur, application idempotente, réconciliation, UI et tests.
**Restant :** tests d’intégration, UX d’erreur, reconnect, piles et or.

## M2 — Preview Reusable — Partiellement implémenté

**Acquis :** composants modulaires, fitting automatique, consommateurs Trading et Character Creation.
**Restant :** leases, arbitrage, tests solver/lifecycle et API interne stable.

## M3 — Alternate Start Character Bootstrap — Implémenté en alpha

**Démo réalisée :** auberge → RaceMenu → classe/kits → build canonique ; buffs ciblés entre deux joueurs.

**Exit criteria validés :** ESP/PSC/PEX versionnés, fallback local, catalogue partagé, inventaire/sorts canoniques, audits conformes.
**Restant avant « Solo complet » :** nouveau jeu automatique, skip Helgen, Valen, sortie et reprise vanilla.

## M4 — Build Persistence and Recovery — Prochain jalon structurel

**Démo :** un build Applied est restauré après reconnexion sans réimporter un personnage arbitraire.

**Exit criteria :** stockage versionné, binding joueur/personnage, migration BuildVersion, snapshot, application idempotente et logs.

## M5 — Group Campaign Start — Futur

**Démo :** 2, 4 puis 10 joueurs créent leur personnage et quittent l’auberge ensemble.

**Exit criteria :** roster, ready check, phases, Valen, Dragonborn secret, départ, late join et performance acceptable.

## M6 — Remaining Character Kits — En cours fonctionnel

**Démo :** toutes les compétences/classes exposées produisent des récompenses réelles et testées.

**Exit criteria :** Invocation, Illusion, Restauration, Enchantement et autres kits matérialisés, équilibrage, previews et tests combinatoires.

## M7 — Contributor Alpha / SDK — Futur

**Démo :** un contributeur externe réalise une petite intégration ou un asset en suivant uniquement la documentation.

**Exit criteria :** runtime générique stabilisé par plusieurs intégrations first-party, exemples, manifests, open roles, reviews et release package.
