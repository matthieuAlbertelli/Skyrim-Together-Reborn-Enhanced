# World Sync — Jalons incrémentaux

## Jalon 0 — Audit et instrumentation

- cartographier les flux actuels ;
- formaliser les invariants ;
- ajouter des logs corrélables ;
- exécuter un scénario à deux joueurs sans changer le gameplay.

**Sortie :** traces client hôte, serveur et client distant démontrant la création locale indépendante des drops.

## Jalon 1 — Identité réseau en mémoire

- introduire `WorldEntityId` ;
- créer un registre local bidirectionnel ;
- attribuer une identité serveur à un drop simple ;
- synchroniser un snapshot minimal.

**Hors périmètre :** persistance disque, ExtraData complexes, cadavres.

## Jalon 2 — Ramassage transactionnel

- commande idempotente de pickup ;
- révision optimiste ;
- arbitrage serveur ;
- suppression synchronisée.

**Critère :** deux joueurs ne peuvent jamais ramasser la même entité.

## Jalon 3 — Cycle de cellule

- conserver l’état logique quand la cellule est vide ;
- snapshot à l’entrée ;
- recréer ou rattacher les références locales.

## Jalon 4 — Stockage durable

- SQLite ;
- migrations ;
- journal ;
- outbox transactionnelle ;
- reprise après crash.

## Jalon 5 — Checkpoints de sauvegarde hôte

- observer save/load ;
- identifier les sauvegardes ;
- coordonner checkpoint STRE et `.ess` ;
- gérer les branches de sauvegarde.

## Jalon 6 — Cadavres

- distinguer acteur et instance de mort ;
- inventaire de cadavre autoritaire ;
- loot transactionnel ;
- persistance et réconciliation.

## Jalon 7 — Cycle de vie vanilla

- reset de cellule ;
- disable/delete ;
- respawn ;
- policies spécialisées par type d’entité.

## Jalon 8 — Données complexes

- piles ;
- santé et charge ;
- enchantements ;
- poison ;
- nom personnalisé ;
- ownership ;
- objets de quête.
