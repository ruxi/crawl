/*
 *  File:       travel.cc
 *  Summary:    Travel stuff
 *  Written by: Darshan Shaligram
 *
 *  Change History (most recent first):
 *
 *     <1>     -/--/--     SD     Created
 */
#ifndef TRAVEL_H
#   define TRAVEL_H

#   include "externs.h"
#   include <stdio.h>
#   include <string>
#   include <vector>
#   include <map>

/* ***********************************************************************
 * Initialises the travel subsystem.
 *
 * ***********************************************************************
 * called from: initfile (what's a better place to initialise stuff?)
 * *********************************************************************** */
void initialise_travel();
void stop_running(void);
void travel_init_new_level();
void toggle_exclude(int x, int y);
void clear_excludes();
unsigned char is_waypoint(int x, int y);
void update_excludes();
bool is_stair(unsigned gridc);
bool is_travelable_stair(unsigned gridc);
command_type stair_direction(int stair_feat);
bool is_player_mapped(unsigned char envch);
command_type direction_to_command( char x, char y );
bool is_resting( void );
bool can_travel_interlevel();

bool is_player_mapped(int grid_x, int grid_y);

void find_travel_pos(int you_x, int you_y, char *move_x, char *move_y, 
                     std::vector<coord_def>* coords = NULL);

/* ***********************************************************************
 * Initiates explore - the character runs around the level to map it. Note
 * that the caller has to ensure that the level is mappable before calling
 * start_explore. start_explore may lock up the game on unmappable levels.
 * If grab_items is true, greedy explore is triggered - in greedy mode, explore
 * grabs items that are eligible for autopickup and visits (previously
 * unvisited) shops.
 *
 * ***********************************************************************
 * called from: acr
 * *********************************************************************** */
void start_explore(bool grab_items = false);

struct level_pos;
void start_translevel_travel(const level_pos &pos);

void start_translevel_travel(bool prompt_for_destination = true);

void start_travel(int x, int y);

command_type travel();

int travel_direction(unsigned char branch, int subdungeondepth);

void prevent_travel_to(const std::string &dungeon_feature_name);

// Sort dungeon features as appropriate.
void arrange_features(std::vector<coord_def> &features);

// Magic numbers for point_distance:

// This square is a trap
#define PD_TRAP             -42

// The user never wants to travel this square
#define PD_EXCLUDED         -20099

// This square is within LOS radius of an excluded square
#define PD_EXCLUDED_RADIUS  -20100

// This square is a waypoint
#define PD_WAYPOINT         -20200

/* ***********************************************************************
 * Array of points on the map, each value being the distance the character
 * would have to travel to get there. Negative distances imply that the point
 * is a) a trap or hostile terrain or b) only reachable by crossing a trap or
 * hostile terrain.
 * ***********************************************************************
 * referenced in: travel - view
 * *********************************************************************** */
extern short point_distance[GXM][GYM];

enum explore_stop_type
{
    ES_NONE               = 0x00,
    ES_ITEM               = 0x01,
    ES_PICKUP             = 0x02,
    ES_GREEDY_PICKUP      = 0x04,
    ES_STAIR              = 0x08,
    ES_SHOP               = 0x10,
    ES_ALTAR              = 0x20
};

////////////////////////////////////////////////////////////////////////////
// Structs for interlevel travel.

// Identifies a level.
struct level_id
{
public:
    int branch;     // The branch in which the level is.
    int depth;      // What depth (in this branch - starting from 1)
    int level_type;

public:
    level_id() : branch(0), depth(-1), level_type(LEVEL_DUNGEON) { }
    level_id(int br, int dep, int ltype = LEVEL_DUNGEON)
        : branch(br), depth(dep), level_type(ltype)
    {
        if (level_type != LEVEL_DUNGEON)
            branch = depth = -1;
    }
    level_id(int ltype) : branch(-1), depth(-1), level_type(ltype) { }

    unsigned short packed_place() const;
    std::string describe(bool long_name = false, bool with_number = true) const;

    bool is_valid() const
    {
        return (branch != -1 && depth != -1) || level_type != LEVEL_DUNGEON;
    }

    // Returns the level_id of the current level.
    static level_id current();

    // Returns the level_id of the level that the stair/portal/whatever at
    // 'pos' on the current level leads to.
    static level_id get_next_level_id(const coord_def &pos);

    bool operator == ( const level_id &id ) const
    {
        return branch == id.branch && depth == id.depth
            && level_type == id.level_type;
    }

    bool operator != ( const level_id &id ) const
    {
        return branch != id.branch || depth != id.depth
            || level_type != id.level_type;
    }

    bool operator <( const level_id &id ) const
    {
        if (level_type != id.level_type)
            return (level_type < id.level_type);
        
        return (branch < id.branch) || (branch==id.branch && depth < id.depth);
    }

    void save(FILE *) const;
    void load(FILE *);
};

// A position on a particular level.
struct level_pos
{
    level_id      id;
    coord_def     pos;      // The grid coordinates on this level.

    level_pos() : id(), pos()
    {
        pos.x = pos.y = -1;
    }

    level_pos(const level_id &lid, const coord_def &coord) 
        : id(lid), pos(coord)
    {
    }

    level_pos(const level_id &lid)
        : id(lid), pos()
    {
        pos.x = pos.y = -1;
    }

    bool operator == ( const level_pos &lp ) const
    {
        return id == lp.id && pos == lp.pos;
    }

    bool operator != ( const level_pos &lp ) const
    {
        return id != lp.id || pos != lp.pos;
    }

    bool operator <  ( const level_pos &lp ) const
    {
        return (id < lp.id) || (id == lp.id && pos < lp.pos);
    }
    
    bool is_valid() const
    {
        return id.depth > -1 && pos.x != -1 && pos.y != -1;
    }

    void save(FILE *) const;
    void load(FILE *);
};

// Tracks items discovered by explore in this turn.
class LevelStashes;
class explore_discoveries
{
public:
    explore_discoveries();
    
    void found_feature(const coord_def &pos, int grid);
    void found_item(const coord_def &pos, const item_def &item);

    // Reports discoveries and prompts the player to stop (if necessary).
    bool prompt_stop() const;

private:
    template <class C> void say_any(const C &coll, const char *stub) const;
    std::string cleaned_feature_description(int feature) const;
    void add_item(const item_def &item);
    
private:
    template <class Z> struct named_thing {
        std::string name;
        Z thing;

        named_thing(const std::string &n, Z t) : name(n), thing(t) { }
    };

    int es_flags;
    const LevelStashes *current_level;
    std::vector< named_thing<item_def> > items;
    std::vector< named_thing<int> > stairs;
    std::vector< named_thing<int> > shops;
    std::vector< named_thing<int> > altars;
};

struct stair_info
{
    coord_def position;     // Position of stair

    level_pos destination;  // The level and the position on the level this
                            // stair leads to. This may be a guess.

    int       distance;     // The distance traveled to reach this stair.

    bool      guessed_pos;  // true if we're not sure that 'destination' is
                            // correct.

    stair_info() : destination(), distance(-1), guessed_pos(true)
    {
        position.x = position.y = -1;
    }

    void reset_distance()
    {
        distance = -1;
    }

    void save(FILE *) const;
    void load(FILE *);
};

// Information on a level that interlevel travel needs.
struct LevelInfo
{
    LevelInfo() : stairs()
    {
        stair_distances = NULL;
    }
    LevelInfo(const LevelInfo &li);

    ~LevelInfo();

    const LevelInfo &operator = (const LevelInfo &other);

    void save(FILE *) const;
    void load(FILE *);

    std::vector<stair_info> &get_stairs()
    {
        return stairs;
    }

    stair_info *get_stair(int x, int y);
    stair_info *get_stair(const coord_def &pos);
    int get_stair_index(const coord_def &pos) const;

    void reset_distances();
    void set_level_excludes();

    const std::vector<coord_def> &get_excludes() const
    {
        return excludes;
    }

    // Returns the travel distance between two stairs. If either stair is NULL,
    // or does not exist in our list of stairs, returns 0.
    int distance_between(const stair_info *s1, const stair_info *s2) const;

    void update();              // Update LevelInfo to be correct for the
                                // current level.

    // Updates/creates a StairInfo for the stair at (x, y) in grid coordinates
    void update_stair(int x, int y, const level_pos &p, bool guess = false);

    // Returns true if the given branch is known to be accessible from the 
    // current level.
    bool is_known_branch(unsigned char branch) const;

    void add_waypoint(const coord_def &pos);
    void remove_waypoint(const coord_def &pos);

    void travel_to_waypoint(const coord_def &pos);
private:
    // Gets a list of coordinates of all player-known stairs on the current
    // level.
    static void get_stairs(std::vector<coord_def> &stairs);

    void correct_stair_list(const std::vector<coord_def> &s);
    void update_stair_distances();
    void fixup();

private:
    std::vector<stair_info> stairs;

    // Squares that are not safe to travel to.
    std::vector<coord_def> excludes;

    short *stair_distances;       // Distances between the various stairs
    level_id id;

    friend class TravelCache;
};

#define TRAVEL_WAYPOINT_COUNT 10
// Tracks all levels that the player has seen.
class TravelCache
{
public:
    void reset_distances();

    // Get the LevelInfo for the specified level (defaults to the current
    // level).
    LevelInfo& get_level_info(unsigned char branch = 0, int depth = -1)
    {
        return get_level_info( level_id(branch, depth) );
    }

    LevelInfo& get_level_info(const level_id &lev)
    {
        LevelInfo &li = levels[lev];
        li.id = lev;
        return li;
    }

    LevelInfo *find_level_info(const level_id &lev)
    {
        std::map<level_id, LevelInfo>::iterator i = levels.find(lev);
        return (i != levels.end()? &i->second : NULL);
    }

    bool know_level(const level_id &lev) const
    {
        return levels.find(lev) != levels.end();
    }

    const level_pos &get_waypoint(int number) const
    {
        return waypoints[number]; 
    }

    int get_waypoint_count() const;

    void set_level_excludes();

    void add_waypoint(int x = -1, int y = -1);
    unsigned char is_waypoint(const level_pos &lp) const;
    void list_waypoints() const;
    void travel_to_waypoint(int number);
    void update_waypoints() const;


    void update();

    void save(FILE *) const;
    void load(FILE *);

    bool is_known_branch(unsigned char branch) const;

private:
    void fixup_levels();

private:
    std::map<level_id, LevelInfo> levels;
    level_pos waypoints[TRAVEL_WAYPOINT_COUNT];
};

int level_distance(level_id first, level_id second);

bool can_travel_to(const level_id &lid);
bool can_travel_interlevel();
bool prompt_stop_explore(int es_why);

extern TravelCache travel_cache;

#endif // TRAVEL_H
