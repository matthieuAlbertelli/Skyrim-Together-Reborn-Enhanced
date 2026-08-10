export interface CharacterRaceDefinition {
  id: string;
  formId: number;
  name: string;
  epithet: string;
  description: string;
  history: string;
  bonuses: readonly string[];
  theme: string;
}

const RACES: readonly CharacterRaceDefinition[] = [
  {
    id: 'race.argonian',
    formId: 0x00013740,
    name: 'Argonien',
    epithet: 'Enfant du Marais Noir',
    description:
      'Adapté aux eaux profondes, aux poisons et aux terres hostiles, l’Argonien survit là où les autres races s’épuisent.',
    history:
      'Les Argoniens sont liés aux Hist, les arbres anciens du Marais Noir. Leur culture, façonnée par les marécages et les invasions, valorise l’endurance, l’adaptation et la mémoire collective.',
    bonuses: [
      'Respiration aquatique',
      'Résistance aux maladies',
      'Peau d’Hist',
      'Affinité avec le crochetage',
    ],
    theme: 'argonian',
  },
  {
    id: 'race.breton',
    formId: 0x00013741,
    name: 'Bréton',
    epithet: 'Sang mêlé de Haute-Roche',
    description:
      'Héritier de lignées humaines et elfiques, le Bréton manie la magie avec naturel tout en résistant mieux que quiconque aux forces occultes.',
    history:
      'Les royaumes de Haute-Roche ont produit des chevaliers, des érudits et des mages de cour. Les Bretons ont appris à survivre au milieu des intrigues féodales et des traditions arcaniques.',
    bonuses: [
      'Résistance à la magie',
      'Peau de dragon',
      'Affinité avec l’invocation',
      'Tradition arcanique polyvalente',
    ],
    theme: 'breton',
  },
  {
    id: 'race.dunmer',
    formId: 0x00013742,
    name: 'Elfe noir',
    epithet: 'Héritier des cendres de Morrowind',
    description:
      'Le Dunmer conjugue maîtrise du feu, ruse et discipline martiale. Il demeure dangereux aussi bien dans l’ombre qu’au cœur d’un duel magique.',
    history:
      'Marqués par l’exode, les guerres et la chute du Mont Écarlate, les Dunmers portent une culture ancienne où les maisons, les ancêtres et les serments ont une importance capitale.',
    bonuses: [
      'Résistance au feu',
      'Courroux ancestral',
      'Affinité avec la destruction',
      'Talents de furtivité et d’armure légère',
    ],
    theme: 'dunmer',
  },
  {
    id: 'race.altmer',
    formId: 0x00013743,
    name: 'Haut-Elfe',
    epithet: 'Descendant de l’archipel de l’Automne',
    description:
      'L’Altmer possède une réserve magique exceptionnelle et une compréhension instinctive des écoles arcaniques.',
    history:
      'Les Hauts-Elfes revendiquent un héritage direct des anciens Aldmers. Leur société accorde une valeur immense à la maîtrise, à la mémoire et à la perfection des arts magiques.',
    bonuses: [
      'Réserve de magie accrue',
      'Sang noble',
      'Affinité avec l’illusion',
      'Maîtrise naturelle des écoles de magie',
    ],
    theme: 'altmer',
  },
  {
    id: 'race.imperial',
    formId: 0x00013744,
    name: 'Impérial',
    epithet: 'Citoyen de Cyrodiil',
    description:
      'L’Impérial associe autorité, discipline et sens pratique. Il excelle à rallier les autres et à préserver l’ordre dans les situations chaotiques.',
    history:
      'Au centre de Tamriel, Cyrodiil a façonné des diplomates, des légionnaires et des administrateurs. L’héritage impérial repose sur l’organisation, la loi et la capacité à unir des peuples différents.',
    bonuses: [
      'Voix de l’Empereur',
      'Chance impériale',
      'Affinité avec la restauration',
      'Formation martiale équilibrée',
    ],
    theme: 'imperial',
  },
  {
    id: 'race.khajiit',
    formId: 0x00013745,
    name: 'Khajiit',
    epithet: 'Voyageur sous les lunes',
    description:
      'Rapide, silencieux et doté de sens aiguisés, le Khajiit domine l’obscurité et les approches furtives.',
    history:
      'Les Khajiits d’Elsweyr vivent au rythme de Masser et Secunda. Marchands, éclaireurs ou guerriers, ils transportent avec eux une culture orale riche et une grande capacité d’adaptation.',
    bonuses: [
      'Vision nocturne',
      'Griffes naturelles',
      'Affinité avec la furtivité',
      'Talents de crochetage et de vol',
    ],
    theme: 'khajiit',
  },
  {
    id: 'race.nord',
    formId: 0x00013746,
    name: 'Nordique',
    epithet: 'Enfant de Bordeciel',
    description:
      'Endurci par le froid et les guerres, le Nordique fait face au danger avec une force brute et un courage difficile à briser.',
    history:
      'Descendants des anciens Atmorans, les Nordiques ont bâti leurs royaumes entre montagnes, tertres et tempêtes. Leurs sagas célèbrent l’honneur, la résistance et les hauts faits guerriers.',
    bonuses: [
      'Résistance au froid',
      'Cri de guerre',
      'Affinité avec les armes à deux mains',
      'Tradition de forge et de combat',
    ],
    theme: 'nord',
  },
  {
    id: 'race.orc',
    formId: 0x00013747,
    name: 'Orque',
    epithet: 'Forgeron des forteresses',
    description:
      'L’Orque transforme la douleur en puissance et se distingue par sa maîtrise des armures lourdes, de la forge et du combat rapproché.',
    history:
      'Les Orques d’Orsinium et des forteresses suivent un code fondé sur le travail, la loyauté et la force. Leur peuple a survécu à de nombreux sièges sans abandonner son identité.',
    bonuses: [
      'Rage du berserker',
      'Accès aux forteresses orques',
      'Affinité avec l’armure lourde',
      'Maîtrise de la forge',
    ],
    theme: 'orc',
  },
  {
    id: 'race.redguard',
    formId: 0x00013748,
    name: 'Rougegarde',
    epithet: 'Lame de Lenclume',
    description:
      'Le Rougegarde est un combattant mobile et infatigable, célèbre pour sa maîtrise des lames et son endurance exceptionnelle.',
    history:
      'Originaires de Yokuda puis établis en Martelfell, les Rougegardes ont forgé une culture guerrière raffinée. Leurs traditions associent honneur martial, navigation et indépendance.',
    bonuses: [
      'Poussée d’adrénaline',
      'Résistance au poison',
      'Affinité avec les armes à une main',
      'Grande endurance martiale',
    ],
    theme: 'redguard',
  },
  {
    id: 'race.bosmer',
    formId: 0x00013749,
    name: 'Elfe des bois',
    epithet: 'Chasseur de Val-Boisé',
    description:
      'Le Bosmer excelle à l’arc, se déplace discrètement et comprend les créatures sauvages mieux que la plupart des peuples de Tamriel.',
    history:
      'Les Elfes des bois vivent sous le Pacte Vert de Val-Boisé. Leur culture privilégie la chasse, la mobilité et une relation étroite avec les forêts et leurs habitants.',
    bonuses: [
      'Commandement animal',
      'Résistance aux poisons et maladies',
      'Affinité avec l’archerie',
      'Talents de furtivité et d’alchimie',
    ],
    theme: 'bosmer',
  },
];

const FALLBACK_RACE: CharacterRaceDefinition = {
  id: 'race.unknown',
  formId: 0,
  name: 'Race inconnue',
  epithet: 'Héritage non répertorié',
  description:
    'Cette race n’est pas encore répertoriée dans le codex de création STRE.',
  history:
    'Son histoire et ses bonus pourront être ajoutés sans modifier le flux de création.',
  bonuses: [],
  theme: 'unknown',
};

export function getRaceDefinition(
  runtimeFormId: number,
  runtimeName: string,
): CharacterRaceDefinition {
  const localFormId = runtimeFormId & 0x00ffffff;
  return (
    RACES.find(race => race.formId === localFormId) ?? {
      ...FALLBACK_RACE,
      name: runtimeName || FALLBACK_RACE.name,
    }
  );
}
