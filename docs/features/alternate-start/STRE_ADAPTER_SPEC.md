# Alternate Start — STRE Integration Spec

> **Statut : Character Build first-party implémenté / adapter de campagne générique proposé**

## Identité

- plugin requis : `STRE_AlternateStart.esp` ;
- intégration actuelle : services C++ first-party compilés ;
- catalogue : `BuildVersion = 5` ;
- adapter ID cible : `stre.alternate-start` ;
- version d’adapter cible : `1`.

## Capabilities actuelles

| Capability | Autorité | État canonique |
|---|---|---|
| `player.character-build/5` | serveur ou fallback local | race, classe, sélections, inventaire, sorts, hashes |
| `player.character-build-state/1` | serveur | revision, Accepted/Applied |
| `player.targeted-buff/1` | moteur/STRE MagicService | buffs alliés reconnus |

Messages actuels :

- `CharacterBuildRequest` ;
- `CharacterBuildResponse` ;
- `CharacterBuildAppliedRequest` ;
- `NotifyCharacterBuildState`.

## Sémantique actuelle

Le client envoie uniquement des identifiants logiques. Le serveur construit le build canonique. Le client applique puis confirme avec les hashes. Le chemin hors ligne réutilise le même catalogue sans serveur.

## Capabilities de campagne cibles

| Capability | Autorité | État canonique |
|---|---|---|
| `campaign.bootstrap/1` | serveur | campagne, roster, manager |
| `character.binding/1` | serveur | personnage autorisé par joueur |
| `campaign.phase/1` | serveur | phase et version |
| `group.ready-check/1` | serveur | ready par joueur |
| `narrative.introduction/1` | serveur | started/completed |
| `campaign.departure/1` | serveur | autorisation |
| `narrative.dragonborn/1` | serveur secret | identité/révélation |

## Intents futurs

- `CreateCampaign`
- `JoinCampaign`
- `BindCharacter`
- `SetReady`
- `RequestIntroductionStart`
- `ReportLocalSceneCompleted`
- `RequestDeparture`

La sélection de classe/build est déjà couverte par le protocole spécifique M7 ; sa migration vers une enveloppe générique n’est pas obligatoire avant que le runtime générique soit stabilisé.

## Application locale

Actuellement :

- nettoyer le personnage ;
- ajouter/équiper les objets ;
- ajouter les sorts ;
- vérifier les hashes ;
- afficher l’état UI.

Futur :

- téléporter au marqueur attribué ;
- lancer/arrêter la scène locale ;
- activer la porte ;
- restaurer la phase après reconnexion.

## Reconnexion

Non implémentée pour les builds. Le futur snapshot devra contenir phase, roster, binding, build, classes, ready states et flags narratifs, puis être appliqué sans rejouer les événements déjà consommés.

## Défaillance

- mismatch de `BuildVersion` : rejet explicite ;
- plugin ou FormID local manquant : rejet explicite ;
- hash inventaire/sorts incorrect : rejet explicite ;
- mode solo hors connexion : fallback local ;
- future campagne adapter incompatible : refuser l’entrée plutôt qu’un état hybride silencieux.
