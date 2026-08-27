import { Component, Input, ViewEncapsulation } from '@angular/core';

@Component({
  selector: 'app-campaign-shell',
  standalone: true,
  templateUrl: './campaign-shell.component.html',
  styleUrls: ['./campaign-shell.component.scss'],
  encapsulation: ViewEncapsulation.None,
})
export class CampaignShellComponent {
  @Input() public screen = '';
}
