# STRE Mod Integration Framework

> **Statut : Spécification proposée**  
> **Patterns :** Microkernel / Plugin Architecture, Ports and Adapters, Adapter, Observer/Event Bus, Command, Strategy, Anti-Corruption Layer

## Objectif

Permettre au développeur d’un mod solo de déclarer :

- quelles fonctionnalités de son mod ont un sens coopératif ;
- quels états STRE doit surveiller ;
- quelles actions sont des intentions ;
- qui possède l’autorité ;
- quelles données sont répliquées ;
- comment appliquer un résultat canonique ;
- comment restaurer un joueur après reconnexion.

Le framework ne rend pas automatiquement tout mod compatible. Il fournit une **ingénierie d’interface** standard pour écrire l’adaptation.

## Vocabulaire

### Capability

Fonctionnalité coopérative nommée et versionnée, par exemple :

- `campaign.start/1`
- `player.class-selection/1`
- `group.ready-check/1`
- `item-preview/1`

### Intent

Demande produite par un joueur ou un mod local : `SelectClass(Paladin)`, `SetReady(true)`, `RequestDeparture`.

### Command

Forme validable et routable de l’intention envoyée à l’autorité.

### Canonical State

État de référence partagé, versionné et sérialisable.

### Event

Résultat accepté et ordonné : `PlayerClassChanged`, `CampaignPhaseChanged`.

### Local Effect

Conséquence appliquée dans Skyrim : équiper un set, lancer une scène, ouvrir une porte.

### Snapshot

État complet nécessaire pour rejoindre ou se resynchroniser.

### Authority Policy

Stratégie indiquant qui décide : serveur, campagne, joueur propriétaire, vote ou leader.

### Replication Policy

Stratégie indiquant qui reçoit la donnée : tous, groupe, propriétaire, rôle privilégié.

## Contrat conceptuel

```cpp
class IStreModAdapter
{
public:
    virtual AdapterDescriptor Describe() const noexcept = 0;
    virtual void RegisterCapabilities(CapabilityRegistry&) noexcept = 0;
    virtual void RegisterIntentHandlers(IntentRegistry&) noexcept = 0;
    virtual void RegisterStateSchemas(StateSchemaRegistry&) noexcept = 0;
    virtual AdapterSnapshot CaptureLocalSnapshot() noexcept = 0;
    virtual ApplyResult ApplyCanonicalSnapshot(const AdapterSnapshot&) noexcept = 0;
    virtual ApplyResult ApplyEvent(const AdapterEvent&) noexcept = 0;
};
```

Ce contrat est directionnel. L’ABI exacte ne doit pas être figée avant le vertical slice Alternate Start.

## Runtime proposé

### Adapter Registry

- identifiant stable ;
- version adapter ;
- version minimale STRE ;
- mod Skyrim requis ;
- dépendances ;
- capabilities ;
- statut actif/incompatible.

### Intent Router

- valide forme, taille et permission ;
- associe player/campaign context ;
- applique rate limits ;
- envoie une commande autoritaire.

### Canonical State Store

- état par campagne et capability ;
- version monotone ;
- hash optionnel ;
- sérialisation ;
- snapshots ;
- migrations.

### Event Dispatcher

- ordre par capability ;
- idempotence ;
- replay depuis snapshot/version ;
- erreurs structurées.

### Policy Engine

Politiques initiales :

- `ServerAuthoritative`
- `PlayerOwnedServerValidated`
- `SessionManagerAuthorized`
- `GroupVote`
- `DragonbornAuthorized`

### Compatibility Gate

Avant activation :

- vérifier plugin et version ;
- vérifier adapter et schema ;
- comparer les hashes/configurations ;
- refuser proprement ou passer en mode solo selon policy.

## Phasage recommandé

### Phase 1 — First-party compile-time

Adapters C++ compilés dans STRE. Messages dédiés ou première enveloppe générique. Alternate Start est le premier exemple.

### Phase 2 — Data-driven

Manifest YAML/JSON, schemas versionnés et bridge Papyrus plus générique.

### Phase 3 — SDK tiers expérimental

API documentée, exemples, validation de manifest, compatibilité de versions.

### Phase 4 — ABI/plugin natif éventuel

Uniquement après plusieurs adapters et besoins stabilisés. Éviter de promettre une ABI C++ binaire trop tôt.

## Manifeste indicatif

```yaml
adapter:
  id: stre.alternate-start
  version: 1.0.0
  requires:
    stre: '>=0.3.0'
    skyrim_plugin: STRE_AlternateStart.esp

capabilities:
  - id: campaign.start
    version: 1
    authority: server
  - id: player.class-selection
    version: 1
    authority: player-owned-server-validated

state:
  schema: alternate-start-state-v1
  snapshot: full
  events: incremental
```

## Contraintes de sécurité

- aucune exécution arbitraire reçue du réseau ;
- adapters identifiés et versions vérifiées ;
- tailles bornées ;
- permissions par capability ;
- données secrètes filtrées par audience ;
- logs sans exposer les secrets narratifs aux clients non autorisés ;
- le serveur ne fait pas confiance aux stages Papyrus déclarés par un client.

## Définition de réussite MVP

Deux clients avec Alternate Start :

1. chargent le même adapter ;
2. rejoignent une campagne ;
3. publient leur état prêt ;
4. reçoivent une phase canonique ;
5. un client se déconnecte ;
6. il revient et applique un snapshot ;
7. la scène ne se rejoue pas deux fois.
