#pragma once

#include "util/point-2d.h"
#include <string>
#include <tl/optional.hpp>

class Direction;
class PlayerType;
struct ProjectResult;
class SpellsMirrorMaster {
public:
    SpellsMirrorMaster(PlayerType *player_ptr);
    void remove_all_mirrors(bool explode);
    void remove_mirror(int y, int x);
    bool mirror_tunnel();
    tl::optional<std::string> place_mirror();
    bool mirror_concentration();
    void seal_of_mirror(const int dam);
    void seeker_ray(const Direction &dir, int dam);
    void super_ray(const Direction &dir, int dam);

private:
    PlayerType *player_ptr;
    Pos2D get_next_mirror_position(const Pos2D &pos_current) const;
    void project_seeker_ray(int target_x, int target_y, int dam);
    void project_super_ray(int target_x, int target_y, int dam);
};
