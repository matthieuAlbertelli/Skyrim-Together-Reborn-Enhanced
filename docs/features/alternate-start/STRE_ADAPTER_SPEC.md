# Alternate Start — STRE Mod Adapter Spec

> **Statut : Spécification de référence**

## Identité

- Adapter ID : `stre.alternate-start`
- Version initiale : `1`
- Plugin requis : `STRE_AlternateStart.esp`
- Mode : first-party compile-time pour le MVP

## Capabilities

| Capability | Autorité | État canonique |
|---|---|---|
| `campaign.bootstrap/1` | serveur | campagne, roster, manager |
| `character.binding/1` | serveur | personnage autorisé par joueur |
| `campaign.phase/1` | serveur | phase et version |
| `player.class-selection/1` | joueur + validation serveur | classe par joueur |
| `group.ready-check/1` | serveur | ready par joueur |
| `narrative.introduction/1` | serveur | started/completed |
| `campaign.departure/1` | serveur | autorisation |
| `narrative.dragonborn/1` | serveur secret | identité/révélation |

## Intents

- `CreateCampaign`
- `JoinCampaign`
- `BindCharacter`
- `CharacterCreationCompleted`
- `SelectClass`
- `SetReady`
- `RequestIntroductionStart`
- `ReportLocalSceneCompleted`
- `RequestDeparture`

## Événements

- `CampaignCreated`
- `PlayerJoined`
- `CharacterBound`
- `CampaignPhaseChanged`
- `PlayerClassChanged`
- `PlayerReadyChanged`
- `IntroductionAuthorized`
- `IntroductionCompleted`
- `DepartureAuthorized`
- `DragonbornRevealed`

## Observation CK

L’adapter surveille seulement des signaux nécessaires : interaction table, fin de création, choix de classe, scène locale terminée, activation porte. Il ne scrape pas en permanence tous les stages.

## Application locale

- téléporter au marqueur attribué ;
- équiper le kit de classe ;
- lancer/arrêter la scène locale ;
- mettre à jour UI ;
- activer la porte ;
- afficher un résumé après reconnexion.

## Reconnexion

Le snapshot contient phase, roster, binding, classes, ready states et flags narratifs. Le client applique la phase sans rejouer les événements consommés.

## Secret Dragonborn

L’identité n’apparaît pas dans le snapshot public. Seul le serveur conserve l’information jusqu’à la révélation. Les capacités spécifiques futures utilisent une audience privée.

## Défaillance

Si l’adapter est incompatible ou absent :

- refuser l’entrée dans une campagne Alternate Start ;
- ne pas dégrader silencieusement vers un état hybride ;
- le mode solo reste disponible hors connexion.
