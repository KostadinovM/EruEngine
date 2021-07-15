#include "Script.h"
#include "Entity.h"
namespace eru::script{
namespace {
utl::vector<detail::script_ptr> entity_scripts;
utl::vector <id::id_type> id_mapping;

utl::vector<id::generation_type> generations;
utl::vector<script_id> free_ids;

using script_registry = std::unordered_map<size_t, detail::script_creator>;
script_registry& registry()
{
	//NOTE: we put this static variable in a function of
	// the initialization order of static data. This way, we can
	// be certain that the data is initialize before accessing it.
	static script_registry reg;
	return reg;
}

bool exists(script_id id) {
	assert(id::is_valid(id));
	const id::id_type index{ id::index(id) };
	assert(index < generations.size() && id_mapping[index] < entity_scripts.size());
	assert(generations[index] == id::generation(id));
	return (generations[index] == id::generation(id)
		&& entity_scripts[id_mapping[index]]				//whether its a valid pointer slot in the id_mapping array
		&& entity_scripts[id_mapping[index]]->is_valid()); //and if its a valid entity
}

} //anonymous namespace

namespace detail
{
u8 register_script(size_t tag, script_creator func)
{
	bool result{ registry().insert(script_registry::value_type{tag, func}).second };
	assert(result);
	return result;
}
}

component create(init_info info, game_entity::entity entity)
{
	assert(entity.is_valid());
	assert(info.script_creator);
	script_id id{};
	//reusing slots
	if (free_ids.size() > id::min_deleted_elements)
	{
		//when reusing ids we take the one from the front and check that it doesn't exist and then remove it so it can be reused
		//hence increase the generation(because we are reusing a slot)
		id = free_ids.front();
		assert(!exists(id));
		free_ids.pop_back();
		id = script_id{ id::new_generation(id) };
		++generations[id::index(id)];
	}
	//new empty id for id_mapping and generations if array is full
	else
	{
		id = script_id{ (id::id_type)id_mapping.size() };
		id_mapping.emplace_back();
		generations.push_back(0);
	}

	assert(id::is_valid(id));
	//the position of where the last entity_script was added so we can point to it via id_mapping array
	const id::id_type index{ (id::id_type)entity_scripts.size() };
	//create an instance that we add to the end of entity_scripts
	entity_scripts.emplace_back(info.script_creator(entity));
	assert(entity_scripts.back()->get_id() == entity.get_id());
	id_mapping[id::index(id)] = index;

	return component{id};
}

void remove(component c)
{
	assert(c.is_valid() && exists(c.get_id()));
	const script_id id{ c.get_id() };
	const id::id_type index{ id_mapping[id::index(id)] };
	//before we swap, we remember what the id of the last script in the array was so we can invalidate its id
	const script_id last_id{ entity_scripts.back()->script().get_id() };
	utl::erase_unordered(entity_scripts, index);
	id_mapping[id::index(last_id)] = index;
	id_mapping[id::index(id)] = id::invalid_id;
}
}