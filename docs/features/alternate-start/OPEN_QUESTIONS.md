# Alternate Start — Questions ouvertes

> **Statut : Décisions restantes après M7**

## Décisions résolues

- le plugin doit fonctionner sans serveur STRE ;
- le client envoie des sélections logiques, jamais l’inventaire final ;
- le serveur construit inventaire et sorts canoniques ;
- les FormIDs sont résolus par plugin + ID local ;
- le nettoyage anti-import est destructif par conception ;
- Crochetage donne 10 crochets vanilla ;
- les tenues d’une même compétence diffèrent visuellement, pas par des bonus sans rapport ;
- l’Enchanteur reçoit une tenue visuelle et des objets magiques désenchantables ;
- les buffs alliés utilisent `Target Actor`, pas `Contact`.

## Persistance et reconnexion

- format de sauvegarde serveur du build ;
- relation entre save Skyrim et snapshot STRE ;
- restauration avant ou après spawn du personnage ;
- migration entre `BuildVersion` ;
- politique si le plugin/catalogue a changé.

## Reset de personnage

- niveaux et XP des 18 compétences ;
- perks acquis et points de perk ;
- historique Santé/Magie/Vigueur ;
- politique sur pouvoirs raciaux et effets permanents de mods.

## Nouveau jeu et skip Helgen

- point d’interception exact ;
- stages/globals vanilla à modifier ;
- déblocage dragons/cris ;
- compatibilité avec autres alternate starts ;
- route de reprise de la quête principale.

## Kits restants

- contenu exact des trois kits d’Enchantement ;
- Invocation, Illusion et Restauration ;
- tenue/kit des compétences utilitaires non matérialisées ;
- équilibrage après tests en jeu ;
- politique de désenchantement et cumul des enchantements faibles.

## Campagne

- modèle de roster et character binding ;
- ready check ;
- cutoff late join ;
- Dragonborn absent ou déconnecté ;
- doublons de classe ;
- changement/respec avant et après départ.

## Dialogues et Valen

- scène locale identique ou synchronisation temporelle ;
- réponses locales, vote ou leader ;
- skip collectif ;
- résumé pour late join ;
- limites à 2, 4 et 10 joueurs.

## Sorts coopératifs

- remplacer l’allowlist nominale de `MagicItem::IsBuffSpell` par keyword/capability ;
- définir friendly fire et stacking ;
- manteaux élémentaires partagés ;
- autorité et réplication des futurs effets scriptés.
