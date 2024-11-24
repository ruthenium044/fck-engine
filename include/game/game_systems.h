#ifndef GAME_UPDATE_SYSTEMS_INCLUDED
#define GAME_UPDATE_SYSTEMS_INCLUDED

void game_cammy_setup(struct fck_ecs *ecs, struct fck_system_once_info *);
void game_spritesheet_setup(struct fck_ecs *ecs, struct fck_system_once_info *);
void game_networking_setup(struct fck_ecs *ecs, struct fck_system_once_info *);

void game_input_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_gameplay_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_animation_process(struct fck_ecs *ecs, struct fck_system_update_info *);
void game_render_process(struct fck_ecs *ecs, struct fck_system_update_info *);

#endif // !GAME_UPDATE_SYSTEMS_INCLUDED
