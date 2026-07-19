# Downed State — Product Spec

> **Statut : Vision validée, non implémentée dans l’archive**

## Automate

```text
En forme
→ Agonie (30 secondes)
→ Hors combat
→ Soins prolongés après la fin du combat
→ En forme
```

## Objectif

Remplacer le respawn immédiat par une conséquence coopérative lisible. Les alliés ont une fenêtre pour intervenir ; après 30 secondes, le joueur reste hors combat jusqu’à la fin de l’affrontement.

## Règles

- soins pendant l’agonie peuvent relever ;
- passé le délai, les soins de combat ne relèvent plus ;
- le joueur hors combat ne revient qu’après fin de combat et soins prolongés ;
- l’état est partagé et autoritaire ;
- déconnexion/reconnexion conserve l’état ;
- pas de téléportation magique automatique.

## Capabilities futures

- `player.down-state/1`
- `group.revive/1`
- `combat.encounter-state/1`
- `injury.persistence/1`
