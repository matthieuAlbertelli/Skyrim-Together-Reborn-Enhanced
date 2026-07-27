# Tests d’acceptation

> **Statut : Trading défini ; Character Build M7 partiellement exécuté ; campagne future**

## Trading — T01 Offre modifiée

**Étant donné** deux joueurs en négociation sur la révision 4
**Et** les deux ont confirmé
**Lorsque** le joueur A modifie sa quantité
**Alors** la révision devient 5
**Et** les deux confirmations sont annulées
**Et** une confirmation portant sur 4 est rejetée sans mutation.

## Trading — T02 Doublon d’application

**Étant donné** un `ApplyId` déjà appliqué avec succès
**Lorsque** la notification est retransmise
**Alors** le client renvoie le même résultat
**Et** l’inventaire n’est pas modifié une seconde fois.

## Trading — T03 Incertitude

**Étant donné** que le premier client a appliqué son plan
**Et** que le second devient indisponible
**Lorsque** le serveur ne peut garantir le résultat global
**Alors** une réconciliation absolue est démarrée ou l’échec est explicitement journalisé
**Et** aucun client ne reste silencieusement divergent.

## Preview — P01 Second consommateur

**Étant donné** que Trading est fermé
**Lorsque** Character Creation sélectionne un objet
**Alors** la preview Skyrim réelle apparaît dans la région Angular
**Et** le cadrage est recalculé après des changements rapides.

## Character Build — C01 Build Mage autoritaire

**Étant donné** un Mage avec une branche Destruction et une branche Altération
**Lorsque** le joueur scelle sa destinée
**Alors** le serveur dérive exactement 7 sorts
**Et** le client applique inventaire et sorts
**Et** les hashes reçus dans l’accusé correspondent
**Et** l’état devient `Applied`.

**État :** smoke-testé en jeu.

## Character Build — C02 Aucun FormID arbitraire

**Étant donné** une requête de build
**Lorsque** le client envoie ses choix
**Alors** la requête contient uniquement race, classe et identifiants logiques
**Et** le serveur construit les objets et sorts depuis son catalogue.

**État :** couvert par architecture/tests.

## Character Build — C03 Buff ciblé

**Étant donné** deux joueurs connectés
**Lorsque** le Mage lance Égide minérale, Souffle aquatique partagé ou Allègement avec `Target Actor`
**Alors** la Magie est consommée une fois
**Et** l’effet est appliqué au joueur ciblé
**Et** il expire après sa durée
**Et** aucun préfixe de load order n’est codé en dur.

**État :** smoke-testé sur deux PC.

## Character Build — C04 Fallback hors ligne

**Étant donné** aucune connexion serveur
**Lorsque** le joueur termine Character Creation
**Alors** le catalogue local applique le build
**Et** l’interface ne reste pas bloquée en attente d’une réponse réseau.

## Alternate Start complet — A01 Solo

**Étant donné** une nouvelle partie sans connexion STRE
**Lorsque** le joueur termine la création à la table
**Alors** il rencontre Valen, choisit une classe et quitte l’auberge
**Et** Helgen n’est pas rejoué
**Et** la quête principale reste accessible.

**État :** non implémenté de bout en bout.

## Alternate Start complet — A02 Groupe

**Étant donné** quatre joueurs liés à la même campagne
**Lorsque** tous ont choisi une classe et sont prêts
**Alors** une seule transition vers `Departure` est acceptée
**Et** la porte devient utilisable chez tous.

**État :** non implémenté.

## Alternate Start complet — A03 Reconnexion

**Étant donné** un joueur déconnecté pendant l’introduction
**Lorsque** il revient après un changement de phase
**Alors** il reçoit le snapshot courant
**Et** la scène complète ne se rejoue pas.

**État :** non implémenté.

## Dragonborn — D01 Secret

**Étant donné** un Dragonborn assigné mais non révélé
**Lorsque** un compagnon inspecte snapshot et logs normaux
**Alors** l’identité n’y apparaît pas.

**État :** spécifié, non implémenté.
