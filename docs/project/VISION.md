# Vision produit STRE

> **Statut : Décision produit**

## Ambition

Créer l’expérience coopérative de Skyrim que des groupes de joueurs pourraient vivre comme une véritable campagne commune : des inconnus deviennent une compagnie, leurs rôles se complètent, leurs décisions produisent un état partagé et le monde reste reconnaissable comme Skyrim.

## Proposition de valeur

STRE apporte trois niveaux de valeur :

1. **Des mécaniques coopératives absentes de STR**, comme le trading sécurisé, l’agonie, la relève et les classes coopératives.
2. **Une campagne structurée**, avec un démarrage collectif, un hub narratif et une progression canonique partagée.
3. **Une plateforme d’intégration**, permettant à un développeur de mod d’expliquer comment la logique solo de son mod doit s’exprimer dans une session STRE.

## Principes non négociables

### Skyrim d’abord

Le joueur doit continuer à reconnaître le rythme, les systèmes et les sensations de Skyrim. STRE ajoute une couche coopérative ; il ne remplace pas le jeu par une logique de MMO.

### Coopération plutôt que punition

Les mécaniques doivent créer des occasions d’aider, protéger, relever, équiper ou coordonner les autres joueurs. La difficulté ne doit pas reposer sur des sanctions arbitraires.

### Autorité explicite

Tout état pouvant diverger entre clients doit avoir une autorité identifiée : serveur, campagne, joueur propriétaire ou politique déclarée.

### Mod solo autonome

Un mod compatible STRE doit pouvoir conserver un mode solo fonctionnel. L’adaptateur STRE enrichit sa sémantique ; il ne doit pas rendre STRE obligatoire lorsque le contenu peut fonctionner seul.

### Modulaire et maintenable

Le cœur réseau, le domaine métier, l’intégration Skyrim, l’UI et le contenu CK doivent rester séparés. Les fonctionnalités doivent être testables indépendamment et documentées par contrat.

### Résilience réseau

Les systèmes doivent tolérer les doublons, retards, pertes temporaires, reconnexions et snapshots. Une scène visible localement ne constitue jamais à elle seule la vérité canonique.

### Compatibilité upstream pragmatique

Le fork doit suivre Skyrim Together Reborn lorsque le coût est raisonnable. Les divergences structurantes doivent être isolées, documentées et faciles à rebaser.

## Public cible

- groupes de 2 à 4 joueurs pour l’expérience principale ;
- capacité technique prévue jusqu’à environ 10 joueurs pour les hubs et événements ;
- moddeurs Creation Kit et Papyrus ;
- développeurs C++ réseau et reverse engineering Skyrim ;
- artistes, scénaristes, comédiens, sound designers et testeurs.

## Ce que STRE n’est pas

- un MMO persistant à grande échelle ;
- une promesse de compatibilité automatique avec tous les mods ;
- une synchronisation naïve de chaque variable locale ;
- un remplacement de la responsabilité du développeur de mod ;
- une campagne centrée exclusivement sur le joueur techniquement hôte.

## Indicateur de réussite

STRE réussit lorsque deux à dix joueurs peuvent commencer ensemble, comprendre qui décide quoi, vivre des systèmes coopératifs sans désynchronisation visible et reprendre leur campagne après une reconnexion sans manipulations manuelles.
