#pragma once

// cstdlib

// external libs

// program

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
namespace SnowGL
{
	/*! @class ParticleSettings
	*	@brief Contains the data for setting the options of a particle system
	*
	*	This struct holds the data required to configure a particle system.
	*/
	struct ParticleSettings
	{
		float		particlesPerSecond		= 0;			/**< The number of particles to emit per second */
		float		lifetimeMin				= 0.0f;			/**< The minimum amount of time a particle will live */
		float		lifetimeMax				= 0.0f;			/**< The maximum amount of time a particle will live */
		float		collisionMultiplier		= 0.0f;			/**< How far ahead of the particle to look for collisions (OLD) */

		glm::vec3	globalWind;								/**< Direction of the global wind modifier */

		glm::vec3	initialVelocity = glm::vec3(0.0f);		/**< The initial velocity given to the particles when they spawn */

		glm::vec3	domainPosition = glm::vec3(0.0f);		/**< The position of the particle domain in 3D space */
		glm::vec3	domainSize = glm::vec3(5.0f);			/**< The size of the particle domain */

		bool		drawDomain = false;						/**< A flag stating whether to draw the particle domain */
		bool		drawPartition = false;					/**< A flag stating whether to draw the partitions for accumulation simulation */

		/** @brief Required particle count getter
		*	@return The maxiumum number of particles that would be required for the system
		*
		*	Calculagtes the maxiumum number of particles that would be required for the system
		*/
		inline int getMaxParticles() const { return (int)ceil(lifetimeMax) * (int)particlesPerSecond; }

		/** @brief Loads the settings from a .ini file
		*	@param _filename The settings file to load
		*
		*	Loads and parses a settings file for particle settings
		*/
		void fromSettingsFile(const std::string &_filename);
	};
}