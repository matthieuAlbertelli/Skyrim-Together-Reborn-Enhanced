# M7 — Intégration des records CK dans le build autoritaire

## Statut

Le correctif de ce jalon intègre dans le catalogue, le protocole réseau et le
client les vêtements créés dans `STRE_AlternateStart.esp`, ainsi que les sorts
de Destruction et d’Altération actuellement disponibles.

Version du catalogue :

```text
BuildVersion = 5
```

Le travail a été contrôlé statiquement sous Linux. Il n’a pas été compilé avec
la toolchain Windows du projet et n’a pas encore été testé dans Skyrim.

## Corrections CK obligatoires avant le test en jeu

L’ESP fourni contient trois anomalies détectées directement dans les records.
Elles doivent être corrigées dans le Creation Kit avant de considérer le lot
conforme.

### 1. Supprimer le sort dupliqué accidentellement

Supprimer uniquement :

```text
STRE_SPEL_Alteration_Protection_AllyMineralAegisDUPLICATE001
```

Conserver :

```text
STRE_SPEL_Alteration_Protection_AllyMineralAegis
```

Le catalogue ne référence que le record conservé, au FormID local
`0x00006FD1`.

### 2. Retirer le retour de ligne du nom de Souffle aquatique partagé

Ouvrir :

```text
STRE_SPEL_Alteration_Exploration_AllyWaterbreathing
```

Remplacer le champ `Name` par exactement :

```text
Souffle aquatique partagé
```

Le nom actuellement enregistré contient un retour de ligne final invisible.

### 3. Corriger les effets d’Égide minérale

Ouvrir :

```text
STRE_SPEL_Alteration_Protection_AllyMineralAegis
```

La liste `Effects` doit contenir exactement une ligne :

```text
Effect    : STRE_MGEF_Alteration_Protection_AllyMineralAegis
Magnitude : 20
Area      : 0
Duration  : 30
Conditions: aucune
```

L’ESP actuel contient quatre lignes utilisant le même `MGEF`. La première a
une durée de `0` et les trois autres ont conservé des conditions de perks de
Peau de chêne. Elles doivent être supprimées puis remplacées par l’unique effet
ci-dessus.

Après ces corrections :

```powershell
Ctrl+S
```

Fermer le Creation Kit, puis exécuter :

```powershell
.\build-and-deploy-dev.ps1

py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

Résultat attendu :

```text
Résultat : conforme.
```

## Équipements intégrés

### Guerrier

Le catalogue accorde automatiquement la tenue de forgeron et ses bottes. Le
choix `warrior.parade.guard_pendant` accorde et équipe le pendentif de garde.
Les choix de matériaux de forge restent inchangés.

### Mage

Le catalogue accorde et équipe la tenue et les bottes d’enchanteur. Ces deux
pièces sont purement visuelles et ne portent aucun enchantement.

### Voleur

Le catalogue accorde automatiquement :

- 10 crochets vanilla ;
- la tenue et les chaussures de foule ;
- la tenue noble et ses bottes ;
- la tenue discrète et ses bottes.

Les anciens choix de trousse, cire et variantes de Furtivité sont retirés du
protocole de sélection.

La tenue d’apothicaire et ses bottes sont présentes dans l’ESP et dans les
aperçus natifs, mais ne sont pas encore accordées par une classe actuellement
exposée dans l’interface.

## Sorts intégrés

Le serveur construit désormais une liste canonique de sorts depuis les choix
logiques du joueur. Le client ne transmet jamais de FormID arbitraire.

### Destruction

```text
Feu
- Flammes (Skyrim.esm)
- Explosion de braises
- Rune de braises

Froid
- Souffle de givre mineur
- Explosion de givre
- Rune de givre mineure

Foudre
- Étincelles mineures
- Décharge explosive
- Rune électrique mineure
```

### Altération

```text
Protection et contrôle
- Peau minérale mineure
- Entrave mineure
- Rune de cendres mineure
- Égide minérale

Exploration et perception
- Lueur mineure
- Détection du vivant mineure
- Détection des morts mineure
- Souffle aquatique partagé

Manipulation de la matière
- Télékinésie mineure
- Transmutation mineure
- Équilibre mineur
- Allègement
```

Pour un mage, chaque combinaison Destruction × Altération produit sept sorts
canoniques : trois sorts de Destruction et quatre sorts d’Altération.

## Protocole autoritaire des sorts

`CharacterBuildSnapshotData` contient maintenant :

```text
CanonicalSpells
SpellHash
```

Le serveur :

1. résout chaque plugin et FormID local depuis le catalogue partagé ;
2. trie et déduplique les sorts ;
3. calcule un hash FNV normalisé ;
4. envoie la liste canonique au client ;
5. valide le `SpellHash` dans `CharacterBuildAppliedRequest`.

Le client :

1. retire les sorts importés pendant le nettoyage existant ;
2. résout les `GameId` autoritaires en FormID chargés ;
3. vérifie que chaque record est un `SpellItem` de type `SPELL` ;
4. exécute `player.addspell XXXXXXXX` via le pont déjà éprouvé
   `Script::CompileAndRun` ;
5. vérifie la présence réelle de chaque sort dans les structures du joueur ;
6. envoie l’accusé d’application avec les hashes d’inventaire et de sorts.

Le même catalogue est appliqué hors ligne, sans serveur STRE.

## Contrôles automatisés ajoutés

```text
Code/tests/character_build.cpp
```

Les tests couvrent :

- les neuf combinaisons Destruction × Altération ;
- l’unicité et le nombre des sorts ;
- le nouveau paquetage simplifié du voleur ;
- les 10 crochets automatiques ;
- la normalisation du hash de sorts ;
- la sérialisation du snapshot et de l’accusé d’application.

Les scripts suivants recoupent le code et l’ESP :

```text
Tools/Scripts/audit_stre_plugin_records.py
Tools/Scripts/audit_character_build_catalog.py
```

Commande de recoupement catalogue/ESP :

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

## Plan de test solo

Après compilation et déploiement :

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Premier test recommandé :

```text
Classe      : Mage
Destruction : Feu
Altération  : Protection et contrôle
```

Résultat attendu après scellement :

- niveau 1 ;
- inventaire importé supprimé ;
- tenue et bottes d’enchanteur équipées ;
- exactement les sept sorts canoniques du choix ;
- Flammes réaccordé après le nettoyage ;
- aucun sort appris importé conservé ;
- aucun échec de résolution de plugin ou de FormID.

Tester ensuite les huit autres combinaisons Destruction × Altération.

## Logs attendus en multijoueur

Serveur :

```text
[STRE][CharacterBuild][Server] Build accepted ... spellCount=7 spellHash=...
[STRE][CharacterBuild][Server] Build applied ... spellHash=... level=1
```

Client :

```text
[STRE][CharacterCreation] Console command executed context=addSpell ...
[STRE][CharacterCreation] Spell grant applied form=...
[STRE][CharacterBuild][Client] Canonical spells applied ... count=7 spellHash=...
[STRE][CharacterBuild][Client] Applied acknowledgement sent ... spellHash=...
```

## Limites connues

- Les builds ne sont toujours pas persistés durablement après reconnexion.
- Le hash garantit la liste canonique et le client vérifie que chaque sort est
  présent. L’absence absolue de tout sort supplémentaire repose encore sur le
  nettoyage anti-import existant.
- Les sorts `Target Actor` doivent être validés à deux joueurs : Égide
  minérale, Souffle aquatique partagé et Allègement.
- Les manteaux élémentaires partagés sont reportés.
- Invocation, Illusion et Restauration restent visibles dans l’interface mais
  leurs récompenses ne sont pas encore accordées.
- Les trois choix de kits d’Enchantement restent à matérialiser dans le
  catalogue ; seule la tenue visuelle est intégrée dans ce jalon.
