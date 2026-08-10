import { CharacterClassDefinition } from '../models/character-creation';

export const CHARACTER_CLASSES: readonly CharacterClassDefinition[] = [
  {
    id: 'class.warrior',
    name: 'Guerrier',
    subtitle: 'Combattant professionnel et polyvalent',
    description:
      "Tous les guerriers sont les combattants professionnels, soldats, mercenaires et aventuriers de Tamriel. Formés au maniement de nombreuses armes et au port de divers types d'armures, ils sont habitués à mettre sans cesse leur vie en jeu et l'effort physique ne leur fait pas peur.",
    majorSkills: ['Parade', 'Forgeage'],
    minorSkills: [
      'Arme à une main',
      'Arme à deux mains',
      'Archerie',
      'Armure lourde',
    ],
    roleTags: ['Première ligne', 'Polyvalence', 'Protection', 'Endurance'],
    tarotTitle: 'Le Rempart',
    archetype: 'Martial',
    loadoutHint:
      "Les armes, l'armure et les outils seront choisis à l'étape suivante.",
    cooperativeGift: 'Don coopératif à définir lors du raffinement des classes.',
    theme: 'warrior',
  },
  {
    id: 'class.mage',
    name: 'Mage',
    subtitle: 'Érudit des arts arcaniques',
    description:
      "Même si la plupart des mages affirment étudier la magie par pur plaisir intellectuel, ils en tirent souvent un profit plus que substantiel. Leur caractère et leur motivation peuvent varier du tout au tout, mais tous ont un point commun : l'amour de la magie.",
    majorSkills: ['Destruction', 'Altération'],
    minorSkills: ['Invocation', 'Illusion', 'Guérison', 'Enchantement'],
    roleTags: ['Arcane', 'Contrôle', 'Polyvalence', 'Soutien'],
    tarotTitle: "L'Arcane",
    archetype: 'Magique',
    loadoutHint:
      "Les sorts, focaliseurs et objets d'étude seront choisis à l'étape suivante.",
    cooperativeGift: 'Don coopératif à définir lors du raffinement des classes.',
    theme: 'mage',
  },
  {
    id: 'class.thief',
    name: 'Voleur',
    subtitle: 'Spécialiste de la ruse et de la subtilité',
    description:
      "Comme son nom l'indique, les voleurs ont pour spécialité de s'emparer des biens des autres mais, contrairement aux brigands, ils font usage de ruse et de subtilité plutôt que de force et de violence, à tel point que certains finissent par acquérir une réputation de redresseur de torts auprès de la population.",
    majorSkills: ['Vol à la tire', 'Crochetage'],
    minorSkills: [
      'Furtivité',
      'Armure légère',
      'Arme à une main',
      'Éloquence',
    ],
    roleTags: ['Infiltration', 'Mobilité', 'Précision', 'Utilitaire'],
    tarotTitle: "L'Ombre",
    archetype: 'Furtif',
    loadoutHint:
      "Les outils, armes discrètes et tenues seront choisis à l'étape suivante.",
    cooperativeGift: 'Don coopératif à définir lors du raffinement des classes.',
    theme: 'thief',
  },
];
