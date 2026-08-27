import { CommonModule } from '@angular/common';
import { Component, Input } from '@angular/core';

export interface CampaignRosterMemberView {
  label: string;
  present: boolean;
}

@Component({
  selector: 'app-campaign-roster',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './campaign-roster.component.html',
  styleUrls: ['./campaign-roster.component.scss'],
})
export class CampaignRosterComponent {
  @Input() public members: CampaignRosterMemberView[] = [];
  @Input() public summary = '';
  @Input() public presentLabel = '';
  @Input() public absentLabel = '';
}
