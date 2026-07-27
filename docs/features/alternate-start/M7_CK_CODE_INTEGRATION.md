# M7 — Intégration des records CK dans le build autoritaire

## Statut

> **Implémenté, compilé et smoke-testé en jeu le 27 juillet 2026.**

Le jalon intègre les vêtements, enchantements et sorts de `STRE_AlternateStart.esp` dans le catalogue partagé, le protocole réseau, le serveur, le client et l’UI.

```text
BuildVersion = 5
```

## Validation effectuée

- build Windows/xmake réussi ;
- manifest strict : 47 records attendus conformes ;
- catalogue/ESP : 41 références conformes ;
- Character Build Mage appliqué en jeu ;
- inventaire et spell hashes acceptés par le serveur ;
- buffs ciblés appliqués et synchronisés sur un autre joueur après correction de l’allowlist STRE.

Les anomalies CK initiales (doublon, nom de Souffle aquatique, effets d’Égide) ont été corrigées et ne sont plus des actions ouvertes.

## Équipements intégrés

### Warrior

- tenue et bottes de forgeron ;
- équipement lourd et choix d’armes vanilla ;
- pendentif de garde faiblement enchanté pour l’option correspondante ;
- kits de matériaux de forge.

### Mage

- tenue et bottes d’enchanteur, purement visuelles ;
- aucune amélioration d’Enchantement portée par la robe ;
- sorts canoniques selon Destruction × Altération.

### Thief

- 10 crochets vanilla ;
- tenue de foule ;
- tenue noble ;
- tenue discrète ;
- armes/équipement selon les sélections.

La tenue d’apothicaire existe dans l’ESP et les previews, mais n’est pas encore accordée par une classe actuellement exposée.

## Sorts intégrés

### Destruction

```text
Feu
- Flammes
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

Chaque combinaison produit 7 sorts canoniques : 3 Destruction + 4 Altération.

## Buffs ciblés coopératifs

Les trois buffs alliés utilisent dans le `SPEL` et le `MGEF` :

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` ne caste pas correctement pour ce type de sort lancé à la main.

Le hook `MagicTarget` de Skyrim Together n’autorise les effets sur un joueur distant que pour les soins ou les sorts reconnus comme buffs. `MagicItem::IsBuffSpell()` résout donc via `STRE_AlternateStart.esp` :

- `STRE_SPEL_Alteration_Protection_AllyMineralAegis` ;
- `STRE_SPEL_Alteration_Exploration_AllyWaterbreathing` ;
- `STRE_SPEL_Alteration_Matter_AllyFeather`.

Les IDs sont locaux au plugin ; aucun préfixe de load order chargé n’est codé en dur.

## Protocole autoritaire des sorts

`CharacterBuildSnapshotData` contient :

```text
CanonicalSpells
SpellHash
```

Le serveur :

1. valide les sélections ;
2. résout plugin et FormID local ;
3. trie et déduplique les sorts ;
4. calcule un hash FNV normalisé ;
5. envoie le snapshot canonique ;
6. valide le `SpellHash` dans l’accusé.

Le client :

1. retire les sorts importés ;
2. résout les `GameId` en FormIDs chargés ;
3. vérifie le type `SpellItem` ;
4. exécute `player.addspell` via `Script::CompileAndRun` ;
5. vérifie la présence réelle ;
6. accuse inventaire et sorts.

Le même catalogue est appliqué hors ligne.

## Contrôles automatisés

`Code/tests/character_build.cpp` couvre :

- les neuf combinaisons Destruction × Altération ;
- unicité et nombre de sorts ;
- package simplifié du Thief ;
- 10 crochets ;
- normalisation du hash ;
- sérialisation snapshot/accusé.

Audits :

```powershell
py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

Résultats validés :

```text
Records attendus : 47
Résultat : conforme.

Références contrôlées : 41
Résultat : conforme.
```

## Logs de succès

```text
[STRE][CharacterBuild][Server] Build accepted ... spellCount=7 spellHash=...
[STRE][CharacterCreation] Spell grant applied form=...
[STRE][CharacterBuild][Client] Canonical spells applied ... count=7 spellHash=...
[STRE][CharacterBuild][Client] Applied acknowledgement sent ... spellHash=...
[STRE][CharacterBuild][Server] Build applied ... spellHash=... level=1
```

Pour un buff distant, le flux magique doit produire les événements de cible/synchronisation STRE correspondants.

## Limites connues

- builds non persistés durablement après reconnexion ;
- reset incomplet des compétences, perks et historiques d’attributs ;
- Invocation, Illusion et Restauration non accordées ;
- kits d’Enchantement non matérialisés ;
- manteaux élémentaires partagés reportés ;
- allowlist des buffs spécifique aux trois FormIDs M7 ;
- validation en jeu encore smoke-level, pas matrice exhaustive de toutes les classes et combinaisons.
