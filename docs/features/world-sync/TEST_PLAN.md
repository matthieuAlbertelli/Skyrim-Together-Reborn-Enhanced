# World Sync — Test plan

> **Statut : scénarios principaux exécutés à deux joueurs; matrice de durcissement à poursuivre**

## Prérequis

Sur les deux clients :

- même build STRE;
- Better Grabbing installé et chargé pour les tests de manipulation;
- configuration compatible avec le build testé.

Serveur :

```ini
[Gameplay]
bEnableItemDrops = true

[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

Conserver pour chaque campagne :

- SHA Git;
- configuration serveur;
- runtime Skyrim;
- versions Better Grabbing / Address Library;
- logs client J1, client J2 et serveur;
- sens du test;
- résultat observé.

## A — Native plugin policy

### A1 — Plugin manquant

Client sans Better Grabbing → connexion rejetée avec erreur native-plugin explicite.

### A2 — Plugin présent

Deux clients avec Better Grabbing chargé → connexion acceptée.

## B — Dynamic dropped WorldEntities

Exécuter J1→J2 puis J2→J1.

### B1 — Drop simple

1. Lâcher un objet simple.
2. L’observateur voit une seule référence.
3. Havok s’exécute localement.
4. Après settlement, les clients convergent.

### B2 — Drop au-dessus d’une surface/obstacle/eau

Vérifier qu’aucune correction continue ne lutte contre Havok et que le transform final converge.

### B3 — Pickup

Après settlement, l’autre joueur ramasse l’objet. Il disparaît chez tous et une seule mutation d’inventaire est appliquée.

## C — Better Grabbing sur WorldEntity dynamique

1. Grab d’un objet synchronisé.
2. L’observateur le voit disparaître.
3. Le joueur peut le déplacer/faire tourner localement pendant plusieurs secondes.
4. Aucun pose intermédiaire ne doit être visible chez l’observateur.
5. Release.
6. L’objet réapparaît/reprend son état distant.
7. Havok local puis settlement final.
8. Pickup possible ensuite.

## D — Lazy adoption d’une référence placée

### D1 — Premier grab

1. Choisir une bouteille/livre/assiette mobile déjà présente dans la cellule.
2. J1 grab.
3. J2 voit la référence locale existante disparaître.
4. J1 déplace/release.
5. J2 voit **la même référence**, pas un duplicate, au transform de release.
6. Settlement final.

### D2 — Re-grab opposé

J2 grab ensuite la même référence. Le serveur doit réutiliser le même `WorldEntityId`.

### D3 — Pickup direct sans grab préalable

Ramasser une autre référence placée. L’adoption et la consommation doivent être atomiques du point de vue serveur, sans double delta d’inventaire.

### D4 — Late join

Déplacer une référence placée, laisser settle, puis connecter l’autre client. Il doit binder sa référence locale existante sans duplicate.

## E — Ownership / vol

### E1 — Objet sans owner

Grab → aucune alarme de vol.

### E2 — Objet possédé avec témoin

Grab → alarme de vol immédiate; gardes/comportement vanilla attendus.

### E3 — Objet autorisé

Si Skyrim considère le joueur owner/autorisé, aucun faux vol.

### E4 — Provenance après inventaire/drop

Voler → inventaire → drop → pickup distant. L’ownership doit rester associé à l’instance supportée.

### E5 — Conteneur possédé

Prendre depuis un conteneur possédé puis dropper. Vérifier la provenance.

## F — Dialogue / arrestation

1. Grab d’un objet possédé devant témoin.
2. Laisser les gardes initier un dialogue.
3. À l’ouverture du Dialogue Menu, l’objet doit être automatiquement relâché.
4. Les choix de dialogue doivent être utilisables immédiatement.
5. Après le dialogue, Better Grabbing doit fonctionner normalement sur un autre objet.
6. Vérifier qu’aucun release réseau ne reste bloqué.

## G — Instance metadata

### G1 — Enchantement vanilla

Drop/pickup d’une arme enchantée avec charge partiellement consommée.

### G2 — Enchantement joueur

Tester un enchantement créé par le joueur, effets + charge.

### G3 — Nom personnalisé

**Limite connue :** ne pas considérer ce scénario validé tant que `ExtraTextDisplayData` n’est pas supporté.

## H — Concurrence / recovery

- deux joueurs tentent de grab la même WorldEntity;
- autorité se déconnecte pendant le grab;
- observateur se déconnecte/reconnecte pendant le grab;
- release très rapide avant fin de lazy adoption;
- changement de cellule après settlement;
- late join après plusieurs drops;
- répétition alternée J1/J2 sur la même référence.

## I — Références à risque

À valider avant extension de support :

- références scriptées;
- enable-parent;
- références de quête;
- ownership faction complexe;
- objets activables avec side effects;
- cellules qui reset.

Ces cas ne doivent pas être annoncés comme garantis avant validation spécifique.
