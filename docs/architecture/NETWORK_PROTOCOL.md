# Règles du protocole réseau STRE

> **Statut : contrat transversal actif**

Ce document définit les **règles communes** du protocole STRE. Les listes de messages, champs et transitions propres à une feature appartiennent à `docs/features/<feature>/PROTOCOL_REFERENCE.md`.

## Modèle général

```text
Client
  → intention / résultat d'application
Server
  → validation / transition canonique
Server
  → notification ou snapshot
Client
  → projection locale Skyrim/UI
```

Le client ne devient pas source de vérité d’un état partagé simplement parce qu’il l’a observé ou affiché.

## Catégories

### Intentions client → serveur

Une intention décrit ce que le joueur tente de faire.

Elle doit contenir uniquement les identifiants et données nécessaires à la validation, avec des collections bornées.

### Résultats/accusés client → serveur

Quand le serveur demande une application locale qui peut échouer, le client peut renvoyer un résultat corrélé afin que le serveur committe, réconcilie ou abandonne explicitement.

### Notifications serveur → client

Une notification décrit une transition admise ou un état canonique. Les clients appliquent la projection locale de manière idempotente lorsque le domaine l’exige.

### Snapshots

Un système reconnectable doit avoir un chemin de snapshot canonique couvrant l’état nécessaire à la reconstruction locale.

## Identité

- les `FormID` chargés Skyrim ne sont pas des identifiants réseau génériques;
- les formulaires persistants utilisent des identités server-space (`GameId`) lorsqu’approprié;
- les instances monde dynamiques utilisent des identités dédiées comme `WorldEntityId`;
- les identifiants de requête/révision doivent être explicites lorsque concurrence ou retransmission l’exigent.

## Bornes et validation

- toute collection décodée possède une borne;
- tout enum reçu est validé;
- toute identité est résolue avant mutation;
- les payloads malformés sont rejetés;
- une feature ne doit pas accepter silencieusement une donnée qu’elle ne sait pas préserver.

## Threading

La réception réseau n’autorise pas directement n’importe quel appel moteur Skyrim.

Les handlers qui doivent muter le jeu doivent marshaller le travail vers un contexte approprié, par exemple le chemin `RunnerService`/`OnUpdate` utilisé par STRE.

## Compatibilité

Tout changement de schéma doit préciser :

- compatibilité client/serveur;
- stratégie de rejet ou de migration;
- tests de round-trip;
- comportement face aux payloads anciens/malformés.

Les changements purement append-only ne sont pas automatiquement sûrs : ils doivent rester cohérents avec les encode/decode des deux côtés.

## Références par feature

- [Trading protocol](../features/trading/PROTOCOL_REFERENCE.md)
- [World Sync protocol](../features/world-sync/PROTOCOL_REFERENCE.md)
- Alternate Start / Character Build : documenter les détails dans son répertoire feature; ne pas recopier la liste ici.

## Future Mod Integration

Une future enveloppe générique d’adapter doit rester versionnée, bornée et négociable. Elle ne remplace pas les protocoles first-party avant d’avoir été validée par plusieurs intégrations réelles.
