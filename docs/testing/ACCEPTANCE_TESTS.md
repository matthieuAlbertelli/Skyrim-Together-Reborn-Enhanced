# Tests d’acceptation

> **Statut : Proposition**

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

## Preview — P01 Réutilisation

**Étant donné** Trading fermé  
**Lorsque** le menu de classe acquiert une lease et sélectionne un item  
**Alors** la preview apparaît dans sa région  
**Et** aucun header ou service nommé Trade n’est requis par le consommateur.

## Preview — P02 Préemption

**Étant donné** une preview passive active  
**Lorsque** une surface prioritaire acquiert la ressource  
**Alors** la première reçoit `Suspended`  
**Et** la session native ne reste pas doublement ouverte.

## Alternate Start — A01 Solo

**Étant donné** une nouvelle partie sans connexion STRE  
**Lorsque** le joueur termine la création à la table  
**Alors** il rencontre Valen, choisit une classe et quitte l’auberge  
**Et** Helgen n’est pas rejoué  
**Et** la quête principale reste accessible.

## Alternate Start — A02 Groupe

**Étant donné** quatre joueurs liés à la même campagne  
**Lorsque** tous ont choisi une classe et sont prêts  
**Alors** une seule transition vers `Departure` est acceptée  
**Et** la porte devient utilisable chez tous.

## Alternate Start — A03 Reconnexion

**Étant donné** un joueur déconnecté pendant `ValenIntroduction`  
**Lorsque** il revient après le passage en `ClassSelection`  
**Alors** il reçoit le snapshot courant  
**Et** la scène complète ne se rejoue pas  
**Et** il peut sélectionner sa classe.

## Dragonborn — D01 Secret

**Étant donné** un Dragonborn assigné mais non révélé  
**Lorsque** un client compagnon inspecte son snapshot et ses logs normaux  
**Alors** l’identité n’y apparaît pas.
