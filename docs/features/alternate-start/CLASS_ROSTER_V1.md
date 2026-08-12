# Alternate Start — Canonical v1 class roster

> **Status: Canonical product roster for STRE v1.0.0**

## Authority and scope

This document defines the **21 character classes required for STRE v1.0.0**, their stable identities, localized French display names, and major/minor skills.

Each class has exactly:

- 2 major skills;
- 4 minor skills.

This definition is independent of implementation status. Warrior, Mage, and Thief are the first vertical slice currently exposed by the catalog and UI; they do not reduce the v1 target roster.

The identifiers, canonical English names, and skill assignments below are normative for v1. Existing runtime IDs remain unchanged. IDs for classes outside the current runtime slice define the stable integration contract to use when those classes are implemented; this documentation change does not add them to gameplay catalogs or UI.

French display names and descriptions are player-facing localization, explicitly separated from canonical engineering identity. Kit details, items, quantities, spells, effects, cooperative abilities, perks, and personal quests evolve in their dedicated implementation and design sources. Historical rewards under `history/` do not become canonical through this document.

Every class must ultimately provide the elements required by [`ROADMAP.md`](../../../ROADMAP.md): starting kits/loadout, cooperative ability or perk, personal quest, UI/catalog/CK/Papyrus/native integration as needed, single-player behavior, and multiplayer validation.

## Combat classes

### Archer

- Stable class ID: `class.archer`
- Canonical English name: Archer
- French localization: Archer

**French localized description**

> Les archers sont des combattants spécialisés dans l'affrontement à distance et les déplacements rapides. Leurs manœuvres incessantes et leur habileté à l'arc leur permettent de maintenir l'ennemi loin d'eux jusqu'à ce qu'il soit suffisamment affaibli, après quoi ils l'achèvent au contact.

**Major skills:** Archery, Light Armor

**Minor skills:** Two-Handed, Sneak, Alteration, Smithing

### Barbarian

- Stable class ID: `class.barbarian`
- Canonical English name: Barbarian
- French localization: Barbare

**French localized description**

> Les barbares sont de fiers guerriers des plaines ou des montagnes. Les civilités ne sont pas leur fort et ils se montrent souvent brutaux et directs. Avides d'actes d'héroïsme, ils excellent dans les combats d'homme à homme.

**Major skills:** Two-Handed, Smithing

**Minor skills:** Sneak, Light Armor, Block, Archery

### Crusader

- Stable class ID: `class.crusader`
- Canonical English name: Crusader
- French localization: Croisé

**French localized description**

> Tout guerrier en armure possédant des pouvoirs magiques et agissant au nom d'une noble cause peut se dire croisé. Les croisés ont pour mission de faire le Bien. Traquant monstres et individus malfaisants, ils s'enrichissent tout en débarrassant le monde des forces du Mal.

**Major skills:** Two-Handed, Restoration

**Minor skills:** Heavy Armor, Block, Destruction, Smithing

### Knight

- Stable class ID: `class.knight`
- Canonical English name: Knight
- French localization: Chevalier

**French localized description**

> Les chevaliers sont des guerriers civilisés, nés de famille noble ou qui se sont distingués en tournoi ou sur le champ de bataille. Sachant lire et écrire, ils suivent les règles de la courtoisie et le code de la chevalerie. Ils étudient l'art de la guerre, mais aussi ceux de la magie et de la guérison.

**Major skills:** One-Handed, Heavy Armor

**Minor skills:** Block, Speech, Restoration, Enchanting

### Rogue

- Stable class ID: `class.rogue`
- Canonical English name: Rogue
- French localization: Roublard

**French localized description**

> Les roublards sont des aventuriers et des opportunistes, aussi doués pour s'attirer les ennuis que pour s'en sortir. Faisant autant confiance à leur charme et à leur panache qu'à leur épée ou à leur sens des affaires, ils n'aiment rien tant que les périodes de conflits, s'appuyant sur leur chance et leur ruse pour survivre en tirant profit de toutes les opportunités qui leur sont offertes.

**Major skills:** Speech, Pickpocket

**Minor skills:** One-Handed, Light Armor, Alchemy, Lockpicking

### Scout

- Stable class ID: `class.scout`
- Canonical English name: Scout
- French localization: Éclaireur

**French localized description**

> Leur grande furtivité permet aux éclaireurs de déterminer les meilleurs itinéraires et d'épier l'ennemi. Quand il leur faut se battre, ils le font à l'aide d'armes de jet ou de traits, en employant des tactiques de guérilla. Contrairement aux barbares, qui se montrent très impulsifs, les éclaireurs combattent de façon prudente et méthodique.

**Major skills:** Sneak, Light Armor

**Minor skills:** Archery, One-Handed, Block, Alteration

### Warrior

- Stable class ID: `class.warrior`
- Canonical English name: Warrior
- French localization: Guerrier

**French localized description**

> Tous les guerriers sont les combattants professionnels, soldats, mercenaires et aventuriers de Tamriel. Formés au maniement de nombreuses armes et au port de divers types d'armures, ils sont habitués à mettre sans cesse leur vie en jeu et l'effort physique ne leur fait pas peur.

**Major skills:** Block, Smithing

**Minor skills:** One-Handed, Two-Handed, Archery, Heavy Armor

## Stealth classes

### Acrobat

- Stable class ID: `class.acrobat`
- Canonical English name: Acrobat
- French localization: Acrobate

**French localized description**

> Les acrobates sont en réalité des cambrioleurs de haut vol. Ces voleurs se reposent sur leur discrétion pour éviter de se faire repérer, et sur leur ruse et leur rapidité pour échapper à la garde le cas échéant.

**Major skills:** Lockpicking, Alteration

**Minor skills:** Sneak, Illusion, Pickpocket, Speech

### Agent

- Stable class ID: `class.agent`
- Canonical English name: Agent
- French localization: Agent

**French localized description**

> Les agents sont des spécialistes du renseignement, formés à jouer différents rôles pour approcher leur cible. Mais ils savent également se défendre au besoin. Farouchement indépendants et habitués à opérer seuls, ils agissent pour le compte d'un supérieur hiérarchique, au nom d'une cause ou encore pour des raisons qui ne regardent qu'eux.

**Major skills:** Illusion, Pickpocket

**Minor skills:** Speech, Sneak, Lockpicking, Conjuration

### Assassin

- Stable class ID: `class.assassin`
- Canonical English name: Assassin
- French localization: Assassin

**French localized description**

> Les assassins sont des tueurs professionnels faisant confiance à leur discrétion et à leurs grandes facultés de mouvement pour approcher de leur cible sans se faire repérer, après quoi ils l'exécutent à l'aide d'une arme de jet ou, s'il leur faut opérer au contact, d'une arme de petite taille. Malgré la profession qu'ils ont choisie, certains agissent pour une noble cause.

**Major skills:** Sneak, Alchemy

**Minor skills:** One-Handed, Archery, Light Armor, Pickpocket

### Bard

- Stable class ID: `class.bard`
- Canonical English name: Bard
- French localization: Barde

**French localized description**

> Les bardes sont des conteurs et des gardiens du savoir. Ils recherchent l'aventure pour les connaissances qu'elle leur apporte, faisant confiance à leurs armes et à leurs sorts pour les protéger contre tous les dangers.

**Major skills:** Speech, Enchanting

**Minor skills:** One-Handed, Illusion, Alchemy, Pickpocket

### Monk

- Stable class ID: `class.monk`
- Canonical English name: Monk
- French localization: Moine

**French localized description**

> Les moines étudient l'art du combat à mains nues. Dotés d'une grande agilité et d'un sens aigu de la discrétion, ils savent également se battre à l'aide de certaines armes de jet ou de corps à corps.

**Major skills:** Alteration, Block

**Minor skills:** One-Handed, Restoration, Sneak, Archery

### Pilgrim

- Stable class ID: `class.pilgrim`
- Canonical English name: Pilgrim
- French localization: Pèlerin

**French localized description**

> Les pèlerins sont des voyageurs en quête d'expériences mystiques. Armes, armures et magie leur permettent de se défendre contre les dangers de la route et leur connaissance approfondie du vaste monde fait d'eux des spécialistes du commerce et de la persuasion.

**Major skills:** Restoration, Archery

**Minor skills:** Two-Handed, Light Armor, Speech, Alchemy

### Thief

- Stable class ID: `class.thief`
- Canonical English name: Thief
- French localization: Voleur

**French localized description**

> Comme son nom l'indique, les voleurs ont pour spécialité de s'emparer des biens des autres mais, contrairement aux brigands, ils font usage de ruse et de subtilité plutôt que de force et de violence, à tel point que certains finissent par acquérir une réputation de redresseur de torts auprès de la population.

**Major skills:** Pickpocket, Lockpicking

**Minor skills:** Sneak, Light Armor, One-Handed, Speech

## Magic classes

### Battlemage

- Stable class ID: `class.battlemage`
- Canonical English name: Battlemage
- French localization: Mage de guerre

**French localized description**

> Les mages de guerre sont des magiciens portés sur les sorts de destruction. Revêtus d'une lourde armure et capables de se défendre l'arme à la main, ils maîtrisent une magie moins diversifiée que les autres lanceurs de sorts et se battent en faisant appel aux éléments et aux créatures convoquées.

**Major skills:** Destruction, Heavy Armor

**Minor skills:** Two-Handed, Smithing, Conjuration, Enchanting

### Healer

- Stable class ID: `class.healer`
- Canonical English name: Healer
- French localization: Guérisseur

**French localized description**

> Les guérisseurs sont des lanceurs de sorts ayant prêté serment de secourir les malades et les blessés. Quand ils se sentent menacés, ils se défendent avec raison, préférant mettre leur adversaire hors de combat et ne le tuant qu'en dernier recours.

**Major skills:** Restoration, Alchemy

**Minor skills:** Alteration, Illusion, Enchanting, Speech

### Mage

- Stable class ID: `class.mage`
- Canonical English name: Mage
- French localization: Mage

**French localized description**

> Même si la plupart des mages affirment étudier la magie par pur plaisir intellectuel, ils en tirent souvent un profit plus que substantiel. Leur caractère et leur motivation peuvent varier du tout au tout, mais tous ont un point commun : l'amour de la magie.

**Major skills:** Destruction, Alteration

**Minor skills:** Conjuration, Illusion, Restoration, Enchanting

### Nightblade

- Stable class ID: `class.nightblade`
- Canonical English name: Nightblade
- French localization: Lame noire

**French localized description**

> Les lames noires utilisent leurs sorts pour augmenter leurs facultés de discrétion, de déplacement et de combat rapproché. Si leur réputation est sinistre, c'est sans doute parce que la plupart d'entre eux font carrière en tant qu'espions, voleurs ou assassins.

**Major skills:** Illusion, Sneak

**Minor skills:** Destruction, One-Handed, Lockpicking, Pickpocket

### Sorcerer

- Stable class ID: `class.sorcerer`
- Canonical English name: Sorcerer
- French localization: Ensorceleur

**French localized description**

> Bien qu'étant des lanceurs de sorts au même titre que les mages, les ensorceleurs se consacrent presque exclusivement aux convocations et aux enchantements. Ils recherchent constamment parchemins, anneaux, armures et armes magiques, et n'aiment rien tant que contrôler morts-vivants et serviteurs daedriques.

**Major skills:** Conjuration, Enchanting

**Minor skills:** Heavy Armor, Destruction, Alteration, Illusion

### Spellsword

- Stable class ID: `class.spellsword`
- Canonical English name: Spellsword
- French localization: Magelame

**French localized description**

> Les magelames sont des lanceurs de sorts apportant leur soutien aux troupes impériales en cas de conflit. Ceux qui quittent l'armée deviennent souvent mercenaires et font d'excellents aventuriers.

**Major skills:** One-Handed, Destruction

**Minor skills:** Block, Heavy Armor, Alteration, Restoration

### Witchhunter

- Stable class ID: `class.witchhunter`
- Canonical English name: Witchhunter
- French localization: Chasseur de sorcières

**French localized description**

> Les chasseurs de sorcières vouent leur existence à l'éradication des cultes malfaisants et de tous les adeptes de la magie profane. Pour ce faire, ils sont formés à toutes les facettes de la lutte contre les vampires, sorcières, sorciers et autres nécromanciens.

**Major skills:** Conjuration, Archery

**Minor skills:** Heavy Armor, Destruction, Enchanting, Lockpicking
