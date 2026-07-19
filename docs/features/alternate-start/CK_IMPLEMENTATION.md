# Alternate Start — Plan d’implémentation Creation Kit

> **Statut : Spécification issue du handoff**

## Records principaux

- `STRE_CELL_AlternateStart`
- `STRE_CELL_DevSandbox`
- `STRE_NPC_Valen`
- `STRE_QUEST_AlternateStart`
- `STRE_SCENE_ValenIntroduction`
- `STRE_DOOR_AlternateStartExit`
- `STRE_TRIG_IntroductionStart`
- `STRE_XMarker_PlayerArrival01` à `10`
- marqueurs table, Valen, aubergiste et sortie

## Phase A — Assainissement

- supprimer PNJ, scènes et marqueurs Whiterun résiduels ;
- auditer scripts, linked refs, enable parents, ownership et persistent refs ;
- conserver RoomMarker, Portals, acoustique, lumière et navmesh tant qu’ils ne sont pas compris ;
- auditer portes et destinations ;
- tester `coc STRE_CELL_AlternateStart`.

## Phase B — Capacité multijoueur

- 10 positions d’arrivée espacées ;
- placements autour de la table ;
- circulation vers portes/chambres ;
- position Valen visible par tous ;
- test collision et caméra.

## Phase C — Squelette solo

- quête et aliases ;
- table de création ;
- dialogue de test ;
- scène courte ;
- classes minimales ;
- porte de sortie ;
- logs Papyrus.

## Phase D — Bridge

Les scripts émettent des intentions via propriétés et API stable. Ils n’appellent pas directement des détails internes du client STRE.

## Navmesh

Toute modification de mobilier imposant un passage doit être suivie d’un test PNJ et multi-joueur. Les portes doivent être reliées des deux côtés et les triangles isolés éliminés.

## Tests solo

- nouvelle partie ;
- création de plusieurs races/sexes ;
- sauvegarde/chargement à chaque phase ;
- sortie et retour ;
- mort/chargement éventuel ;
- absence de STRE ;
- installation/désinstallation contrôlée.
