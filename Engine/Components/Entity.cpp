#include "Entity.h"
#include <vector>
#include "Transform.h"
#include "Script.h"

namespace eru::game_entity {
namespace {
utl::vector<transform::component> transforms;
utl::vector<script::component> scripts;
utl::vector<id::generation_type> generations;
utl::deque<entity_id> free_ids;
} // anonymous namespace


entity create(entity_info info)
{
	assert(info.transform); //all games must have a transform component
	if (!info.transform) return entity{};
	entity_id id;

	if (free_ids.size() > id::min_deleted_elements) //reuse slots from array
	{
		id = free_ids.front();
		assert(!is_alive(id));
		free_ids.pop_front();
		id = entity_id{ id::new_generation(id) };
		++generations[ id::index(id) ];
	}
	else  //add new slot at the end of the array
	{
		id = entity_id{ (id::id_type)generations.size() };
		generations.push_back(0);

		//Resize components
		//NOTE: we don't call resize(), so the number of memory allocations stays low
		transforms.emplace_back();
	}
	
	const entity new_entity{ id };
	const id::id_type index{ id::index(id) };

	//Create transform component
	assert(!transforms[index].is_valid());
	transforms[index] = transform::create(*info.transform, new_entity);
	if (!transforms[index].is_valid()) return {}; //returns invalid id
	
	// Create Script component
	if (info.script && info.script->script_creator)
	{
		assert(!scripts[index].is_valid());
		scripts[index] = script::create(*info.script, new_entity);
		assert(scripts[index].is_valid());
	}

	return new_entity;
}

void remove(entity_id id)
{
	const id::id_type index{ id::index(id)};
	assert(id::is_valid(id));
	transform::remove(transforms[index]);
	transforms[index] = {}; //returns invalid id
	free_ids.push_back(id); //add the id that's been freed so it can be reused later on

}

bool is_alive(entity_id id)
{
	assert(id::is_valid(id));
	const id::id_type index{ id::index(id) };
	//index is within length of generations (length of generation is indication of number of slots for entities to exist in)
	assert(index < generations.size());
	assert(generations[index] == id::generation(id));
	//whether the generation of this entity equals the generations of the slot that we have for this entity + its transform is valid
	//=> if thats true, its alive otherwise it's been replaced by another entity
	return (generations[index] == id::generation(id) && transforms[index].is_valid());
}

transform::component entity::transform() const
{
	//check if generations[index] == id::generation(_id)
	//check if is_valid()
	assert(is_alive(_id));

	const id::id_type index{ id::index(_id) };
	return transforms[index];
}

script::component entity::script() const
{
	assert(is_alive(_id));

	const id::id_type index{ id::index(_id) };
	return scripts[index];
}

}