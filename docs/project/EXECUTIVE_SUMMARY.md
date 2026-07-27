# Synthèse exécutive

> **Statut : État produit et architecture au 27 juillet 2026**

## Où en est le projet

STRE possède deux preuves techniques crédibles :

- **Trading** : domaine, protocole, serveur autoritaire, idempotence, réconciliation, UI et preview 3D.
- **Alternate Start / Character Build** : plugin CK versionné, RaceMenu/Angular, catalogue partagé, inventaire et sorts canoniques, hashes, application client, mode hors ligne et buffs ciblés coopératifs.

La preview 3D est réutilisée par Trading et Character Creation. Elle constitue une base interne réelle, mais pas encore un SDK tiers stable.

## Ce qui est effectivement validé

- `BuildVersion = 5` ;
- 47 records CK attendus conformes au manifest M7 ;
- 41 références catalogue/preview recoupées avec l’ESP ;
- build Windows réussi ;
- bootstrap Mage smoke-testé dans Skyrim ;
- Égide minérale, Souffle aquatique partagé et Allègement fonctionnels sur un autre joueur après adaptation du filtre de buffs STRE.

## Ce qui doit être communiqué avec précision

- Alternate Start est un **vertical slice de création et de build**, pas encore un remplacement complet du nouveau jeu.
- Le serveur est autoritaire pour l’inventaire et les sorts pendant la session, mais les builds ne sont pas encore persistés durablement.
- Campaign State, roster, ready check, Valen, départ partagé et reconnexion restent à implémenter.
- La compatibilité avec « tout mod » signifie qu’un développeur pourra écrire un adapter ; STRE ne synchronise pas automatiquement n’importe quel mod.
- Invocation, Illusion et Restauration restent des conceptions visibles dans l’UI, pas des récompenses appliquées.

## Décision structurante

Poursuivre l’approche first-party : stabiliser les contrats à travers Trading, Character Build et les prochaines fonctions Alternate Start avant de publier un SDK générique. Le plugin CK conserve un chemin solo ; les données importantes sont dérivées et validées par STRE en multijoueur.

## Prochaines actions recommandées

1. Persister et restaurer les builds après reconnexion.
2. Terminer les kits d’Enchantement et les écoles de magie restantes.
3. Implémenter le skip Helgen et valider la reprise vanilla.
4. Ajouter Valen, le départ et une phase de campagne minimale.
5. Transformer les buffs coopératifs en capability extensible plutôt qu’en allowlist nominale.
6. Faire évoluer la preview vers un lease manager.
7. Compléter la matrice de test solo et deux joueurs avant d’augmenter le nombre de classes.

## Sources de vérité

- état actuel : `docs/audit/CURRENT_STATE_AUDIT.md` ;
- implémentation M7 : `docs/features/alternate-start/M7_CK_CODE_INTEGRATION.md` ;
- catalogue : `Code/common/CharacterCreation/CharacterBuildCatalog.*` ;
- présentation UI : `Code/skyrim_ui/src/app/data/character-loadouts.ts` ;
- records CK : `CK_RECORDS_M7_IMPLEMENTED.json` et `STRE_AlternateStart.esp`.
