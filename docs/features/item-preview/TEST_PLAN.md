# Item Preview — Test Plan

> **Statut : Smoke-tested dans Trading et Character Creation ; tests automatisés à compléter**

## Déjà observé en jeu

- objets réels chargés dans Trading ;
- objets réels chargés dans Character Creation ;
- cadrage automatique ;
- changements rapides de sélection ;
- régions Angular mises à jour ;
- reload natif lorsque nécessaire.

## Solver pur

- modèles très petits/grands ;
- centres décalés ;
- edges ;
- scale min/max ;
- invalid bounds ;
- convergence après raffinements.

## Runtime

- begin/end idempotents ;
- host show/hide réordonné ;
- item change pendant mesure ;
- region revision stale ;
- reload pending annulé ;
- manager absent ;
- bind concurrent ;
- future lease preemption ;
- passage Trading → Character Creation → Trading.

## Rendu

- 1080p, 1440p, 4K ;
- 16:9, 21:9 ;
- UI scale ;
- armes, armures, livres, potions, petits objets ;
- objets avec bounds atypiques ;
- tenues STRE ;
- conflit avec inventaire/crafting menus ;
- comportement explicite pour les sorts sans modèle 3D utile.
