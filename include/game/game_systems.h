#ifndef GAME_UPDATE_SYSTEMS_INCLUDED
#define GAME_UPDATE_SYSTEMS_INCLUDED

void game_authority_controllable_create(struct fck_ecs *ecs, struct fck_system_once_info *);
void game_networking_setup(struct fck_ecs *ecs, struct fck_system_once_info *);

void game_input_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_gameplay_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_render_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_demo_ui_process(struct fck_ecs* ecs, struct fck_system_update_info*);

#endif // !GAME_UPDATE_SYSTEMS_INCLUDED
