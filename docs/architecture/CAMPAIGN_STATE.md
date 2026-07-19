# Modèle d’état de campagne

> **Statut : Spécification proposée**

## Objectifs

- une source de vérité pour le groupe ;
- reprise après reconnexion ;
- distinction Session Manager / Dragonborn ;
- transitions explicites ;
- évolution de schéma ;
- filtrage des informations secrètes.

## Modèle conceptuel

```cpp
struct CampaignState
{
    CampaignId Id;
    CampaignSchemaVersion SchemaVersion;
    StateVersion Version;
    CampaignPhase Phase;

    PlayerId SessionManager;
    std::optional<PlayerId> Dragonborn;
    bool DragonbornRevealed;

    std::vector<CampaignPlayerState> Players;
    AdapterStateMap AdapterStates;

    bool IntroductionStarted;
    bool IntroductionCompleted;
    bool DepartureAuthorized;
};
```

```cpp
struct CampaignPlayerState
{
    PlayerId Player;
    CharacterBinding Character;
    ConnectionState Connection;
    CampaignRole Role;
    std::optional<ClassId> SelectedClass;
    bool Ready;
    bool IntroductionCompleted;
};
```

## Phases

```text
Lobby
→ CharacterCreation
→ Arrival
→ Gathering
→ ValenIntroduction
→ ClassSelection
→ ReadyCheck
→ Departure
→ OpenWorld
```

Une phase peut avoir des sous-états internes, mais le modèle public doit rester lisible.

## Transition

Chaque transition définit :

- phase source autorisée ;
- préconditions ;
- command actor ;
- authority policy ;
- mutations ;
- événement ;
- conséquence locale ;
- stratégie de reprise.

Exemple `AuthorizeDeparture` :

- source : `ReadyCheck` ;
- préconditions : tous les joueurs obligatoires prêts, classes valides, introduction terminée ;
- autorité : serveur ;
- mutation : `Phase=Departure`, `DepartureAuthorized=true`, `Version++` ;
- événement : `CampaignPhaseChanged` ;
- effet local : activation de la porte.

## Snapshot et audience

Le serveur produit au moins deux vues :

- **snapshot public de campagne** ;
- **snapshot privé** pour les données réservées, comme l’identité du Dragonborn avant révélation.

Un client non autorisé ne doit pas recevoir la donnée secrète masquée par l’UI ; elle ne doit pas être envoyée.

## Binding de personnage

Le personnage est lié à la campagne par une identité créée pendant le bootstrap. Le modèle doit contenir :

- identifiant de campagne ;
- identifiant joueur ;
- empreinte du personnage ;
- statut créé/validé ;
- classe ;
- version de bootstrap.

Un personnage externe ne peut pas rejoindre sans procédure d’import explicitement conçue.

## Reconnexion

1. authentification ;
2. vérification du binding ;
3. négociation des adapters ;
4. snapshot public + privé ;
5. application idempotente ;
6. reprise de la phase locale ;
7. événements post-snapshot seulement à partir de `Version+1`.

## Late join

Politique MVP recommandée : autorisé jusqu’à `ReadyCheck`, refusé après `Departure` sauf mécanisme de rattrapage ultérieur.

## Persistance

Le snapshot de campagne doit être sérialisable dès le MVP. Stockage recommandé : fichier atomique serveur par campagne au départ, puis base intégrée si besoin. La sauvegarde Skyrim locale reste une conséquence, pas l’unique source canonique.
