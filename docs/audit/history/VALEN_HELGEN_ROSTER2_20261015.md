# Valen Helgen — smoke roster-2 humain du 15 octobre 2026

> **Statut : Historical — attestation humaine datée.**
> L'état courant de validation appartient à [STATUS](../../project/STATUS.md).

## Source de la preuve

Après la checklist manuelle A→B puis B→A, le mainteneur a déclaré dans cette conversation :

> tout est PASS, aucune erreur

Cette attestation globale couvre les deux directions demandées. Les résultats
ci-dessous sont transcrits comme **PASS humains**. L'agent n'a pas exécuté le
smoke ou analysé de nouveaux logs, captures ou exports console fournis avec
cette attestation. Aucun horaire de partie, identifiant de campagne ou paquet
réseau n'est inventé.

Build désigné : `e6a182138a17d8d143fbfd0976a76b34734ddfb3`, branche
`feat/valen-helgen-lead`. Client attendu et vérifié au build/deploy et préflight :
`D4CF33B57472EB51D6BE7C5A052D67E61B7C80A809252228B91716D67C44DA3F`.
A = poste local ; B = PC-Vero, correspondance confirmée par le mainteneur.

## Protocole soumis aux joueurs humains

Processus clients neufs, campagne roster-2 propre, création terminée normalement
et aucun joueur ayant déjà accepté Valen. Avant acceptation : quête narrative
avant stage 10, branche Valen/Helgen disponible et absence de quête prématurée
au journal. L'initiateur choisit « Nous irons peut-être voir ce qui s'y est passé. »
Le receveur doit obtenir la quête sans parler à Valen ni accepter lui-même,
puis constater que l'acceptation n'est plus proposée lorsqu'il lui parle.

Pour B→A, fermer complètement les deux clients et le serveur, puis utiliser
une nouvelle campagne propre ; ne pas réutiliser la campagne déjà au stage 10.
L'option expérimentale Misc/None reste désactivée. Seules commandes console :

```text
getstage STRE_QUEST_ValenHelgen
sqv STRE_QUEST_ValenHelgen
```

Aucun setstage/startquest/stopquest/resetquest pour produire les résultats.

## Résultats attestés

Chaque PASS signifie « selon l'attestation du mainteneur », sans observation
automatisée indépendante.

| Contrôle | A→B : A initiateur, B receveur | B→A : B initiateur, A receveur |
|---|---|---|
| Initiateur : dialogue et réponse finale corrects | PASS humain | PASS humain |
| Initiateur : Les ruines de Helgen visible | PASS humain | PASS humain |
| Initiateur : objectif 10 visible et actif | PASS humain | PASS humain |
| Initiateur : stage 10 | PASS humain | PASS humain |
| Initiateur : quête running | PASS humain | PASS humain |
| Receveur sans interaction : quête reçue automatiquement | PASS humain | PASS humain |
| Receveur : objectif 10 visible et actif | PASS humain | PASS humain |
| Receveur : stage 10 | PASS humain | PASS humain |
| Receveur : quête running | PASS humain | PASS humain |
| Receveur : option Valen disparue après synchronisation | PASS humain | PASS humain |
| Aucun redémarrage ni doublon de journal/objectifs | PASS humain | PASS humain |
| Technique Helgen non régressée dans le périmètre du smoke | PASS humain | PASS humain |
| Absence de corruption ou seconde création de personnage | PASS humain | PASS humain |
| Crash / erreur signalé | Non | Non |

## Traçabilité technique distincte de l'attestation

Le workflow de build/deploy a terminé sur le build désigné avec 17 imports CK
IDENTIQUE et un manifest inchangé de 27 fichiers. Les huit compagnons validés
sont restés byte-identiques et les trois backups Vortex physiquement intacts,
exclus de la collecte/import/manifest. Hashes de référence :

| Artefact | SHA256 |
|---|---|
| STRE_AlternateStart.esp | `0058D2BD28C5BBEF5CCC14D09767EB2CB1BB8C22D81F3604C1D02187E01DB97F` |
| SkyrimTogether.esp, protégé | `3196266AA29855574BDA48335DA9AB2F18AD33A80EEE116400DA7C5C81C93D7E` |
| Manifest de packaging | `75C3CEC99C6CF156DA376473237E68F768B71F871DC0666D39AF01D4A234FB4E` |

CI du build testé : [Windows PASS](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38719181490)
et [Linux PASS](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38719181617).

Une tentative de préparation antérieure par l'agent avait démarré le serveur
et envoyé une demande de lancement du client avant la clarification « humains
uniquement ». Elle ne constitue pas un smoke. Le serveur a ensuite été arrêté
à la demande du mainteneur pour libérer Vortex ; aucun processus de jeu ne
restait alors actif. Aucun PASS ne provient de cette tentative.

## Conclusion bornée et arrêt

**A→B PASS humain ; B→A PASS humain ; aucune erreur signalée.** Le démarrage et
le stage/objectif 10 sont acceptés dans ce smoke. Les étapes narratives
ultérieures, la propagation de Stop et la récupération de campagne ne sont pas
testées ici.

Cette consignation est documentaire : aucun lancement, changement de code ou
contenu CK, import ou rebuild. Un commit documentaire ultérieur n'est pas
présenté comme le build testé en jeu. La [PR #95](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/pull/95)
reste DRAFT et l'[issue #94](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/94)
reste ouverte, sans Ready, merge ou auto-merge. **STOP pour décision finale du mainteneur.**
