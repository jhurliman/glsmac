return {
	refresh: () => {
		if (this.p.game.get_smac_eliminated()) {
			this.p.modules.menu.close_all();
			this.window.show();
		} else {
			this.window.hide();
		}
	},
	init: (p) => {
		this.p = p;
		this.window = p.root.window({class: 'game-popup', width: 480, height: 160, blocker: 'rgba(0, 0, 0, 180)'});
		this.window.text({class: 'game-popup-text', top: 16, left: 16, right: 16, text: 'Your faction has been eliminated.'});
		const menu = this.window.button({class: 'game-popup-button', bottom: 16, left: 16, right: 16, height: 32, text: 'Return to Main Menu'});
		menu.on('click', () => { this.p.glsmac.reset(); return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {} && e.code == 'ENTER') { this.p.glsmac.reset(); }
			return true;
		});
		this.window.listen(p.game, 'turn', (e) => { this.refresh(); });
		this.refresh();
	},
};
