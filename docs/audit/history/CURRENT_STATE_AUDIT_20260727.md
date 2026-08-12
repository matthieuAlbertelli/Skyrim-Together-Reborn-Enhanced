# Audit de l’état actuel

> **Statut : Snapshot historique au 27 juillet 2026 — non canonique pour l'état
> courant.** Voir [`docs/project/STATUS.md`](../../project/STATUS.md).
> **Baseline historique :** audit source du 19 juillet 2026
> **Version déclarée :** `0.1.0-alpha.1`

## Résumé exécutif

STRE possède désormais deux verticales techniques actives :

1. un système de trading joueur-à-joueur autoritaire avec saga de réconciliation ;
2. un bootstrap de personnage Alternate Start combinant plugin Creation Kit, UI Angular/CEF, catalogue partagé et validation serveur.

Le second point n’était pas présent dans l’archive du 19 juillet. Il a depuis été intégré au dépôt, compilé sous Windows et smoke-testé dans Skyrim, notamment pour les buffs ciblés entre deux PC.

## Trading

Le trading comprend toujours :

- un domaine métier indépendant ;
- un service serveur autoritaire ;
- un protocole dédié et borné ;
- des plans de mutation déterministes ;
- une application client idempotente ;
- une réconciliation vers des quantités absolues ;
- une UI Angular/CEF ;
- une preview 3D native modulaire ;
- des tests de domaine et de sérialisation.

Le modèle doit être décrit comme une **saga autoritaire compensée**, pas comme une transaction ACID distribuée.

### Limites Trading

- état de session non persisté à travers un redémarrage serveur ;
- reconnect en cours d’échange encore à valider complètement ;
- pas de stack splitting ni d’échange d’or ;
- preview toujours non publiée comme SDK tiers stable.

## Item Preview

Les composants internes incluent notamment :

- `ItemPreviewController` ;
- `ItemPreviewNativeSession` ;
- `ItemPreviewHostSession` ;
- `ItemPreviewHostBridge` ;
- `ItemPreviewFitSolver` ;
- `ItemPreviewRasterMeasurer` ;
- `TradePreviewHostMenu`.

La plateforme a désormais un second consommateur first-party dans l’écran Character Creation. Cela valide sa réutilisabilité interne, sans résoudre encore le besoin d’un lease manager multi-consommateurs ni d’une API tierce stable.

## Alternate Start — état implémenté

### Plugin Creation Kit

Les fichiers authored sont versionnés sous `GameFiles/Skyrim` :

- `STRE_AlternateStart.esp` ;
- `Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc` ;
- `Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex`.

Éléments confirmés :

- cellules `STRE_CELL_AlternateStart` et `STRE_CELL_DevSandbox` ;
- quête `STRE_QUEST_AlternateStart` ;
- étapes `0`, `10` et `20` ;
- aliases joueur et sièges ;
- déplacement puis assise du joueur ;
- déclenchement de RaceMenu et de Character Creation ;
- records custom d’équipement, enchantements, sorts et effets magiques.

Le manifest strict `CK_RECORDS_M7_IMPLEMENTED.json` valide 47 records attendus. L’audit catalogue/ESP valide 41 références utilisées par le code.

### Character Creation

Le client expose `UiSurface::CharacterCreation` et un `CharacterCreationService` qui orchestre :

- RaceMenu ;
- choix Warrior, Mage ou Thief ;
- groupes de loadouts ;
- preview 3D d’objets Skyrim réels ;
- résumé ;
- soumission finale ;
- chemin local hors ligne ou chemin serveur autoritaire.

### Build autoritaire

Le catalogue courant est :

```text
BuildVersion = 5
```

Le serveur dérive depuis les identifiants logiques :

- l’inventaire canonique ;
- le hash d’inventaire ;
- la liste canonique de sorts ;
- le hash de sorts ;
- les métadonnées d’équipement.

Le client nettoie le personnage importé, applique le snapshot canonique, vérifie les objets et sorts réellement présents, puis envoie `CharacterBuildAppliedRequest`. Le serveur valide les deux hashes avant de marquer le build `Applied` et de fixer le niveau serveur à 1.

Le même catalogue est utilisé en mode hors ligne sans dépendance obligatoire au serveur.

### Sorts Mage

Les choix Destruction et Altération sont matérialisés :

- 3 branches de Destruction, 3 sorts chacune ;
- 3 branches d’Altération, 4 sorts chacune ;
- 7 sorts canoniques pour chaque combinaison Mage.

Les buffs suivants sont explicitement reconnus par le hook magie STRE et ont été smoke-testés sur un joueur distant :

- Égide minérale ;
- Souffle aquatique partagé ;
- Allègement.

### Nettoyage anti-import

Le flux actuel retire l’inventaire et la magie importés, dissipe les effets temporaires, remet le niveau à 1 et applique le build canonique. Il ne remet pas encore à zéro :

- les niveaux/XP des 18 compétences ;
- les perks et points de perk ;
- l’historique d’augmentation Santé/Magie/Vigueur.

## Limites Alternate Start

- le nouveau jeu n’est pas encore automatiquement redirigé de bout en bout vers l’auberge ;
- le skip Helgen et la reprise exhaustive des quêtes vanilla restent à implémenter/tester ;
- Valen, la scène d’introduction et la sortie narrative ne sont pas terminés ;
- roster, ready check, Campaign State, late join et Dragonborn secret ne sont pas implémentés ;
- les builds ne sont pas persistés durablement après reconnexion ou redémarrage serveur ;
- Invocation, Illusion et Restauration restent visibles dans l’UI mais sans récompenses canoniques ;
- les kits d’Enchantement et plusieurs kits de compétences restent à matérialiser ;
- les tests en jeu réalisés sont des smoke tests, pas encore une validation exhaustive des neuf combinaisons Mage et de toutes les classes.

## Architecture réellement validée

Le dépôt démontre aujourd’hui deux patrons first-party autoritaires :

- saga de trading avec réconciliation ;
- build de personnage avec snapshot canonique et accusé d’application.

Le Mod Integration Framework générique, le bridge Papyrus public et Campaign State restent des architectures proposées. Il ne faut pas présenter le service Character Build comme un SDK générique déjà stabilisé.

## Recommandations immédiates

1. Automatiser le test des neuf combinaisons Mage dans le build natif et compléter les tests en jeu.
2. Implémenter la persistance/reconnexion des builds avant d’étendre l’autorité à la campagne complète.
3. Terminer les kits restants à partir du catalogue et du tableur V2.
4. Mettre en place le skip Helgen et la reprise vanilla avant d’annoncer un Alternate Start complet.
5. Ajouter Valen, le départ et le Campaign State par petits jalons testables.
6. Remplacer l’allowlist nominale des buffs par une classification plus extensible avant un SDK tiers.
7. Continuer l’évolution de la preview vers un gestionnaire de leases.

## Traçabilité historique

L’audit du 19 juillet 2026 constatait uniquement Trading/Preview et l’absence d’Alternate Start dans l’archive. Ce constat reste valable pour cette archive historique, mais il est superseded par l’état décrit ci-dessus. Les détails de la baseline upstream restent dans `UPSTREAM.md`.
