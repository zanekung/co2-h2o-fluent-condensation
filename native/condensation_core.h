#ifndef FIG2_CONDENSATION_CORE_H
#define FIG2_CONDENSATION_CORE_H

/* Research candidate, not an experimentally validated model.
 * Gas: ideal H2O + CO2. UDS0: kg liquid / kg gas.
 * UDS1: number / (kg gas * CO2H2O_NUMBER_SCALE).
 * All core number variables are unscaled; only the Fluent wrapper scales them.
 */
#define CO2H2O_NUMBER_SCALE 1.0e15
#define CO2H2O_CP_VAPOR 1864.0
#define CO2H2O_CP_LIQUID 4181.3
#define CO2H2O_CP_CO2 846.0
#define CO2H2O_TREF 298.15
#define CO2H2O_LREF 2441700.0

typedef struct {
    double temperature_k, pressure_pa, gas_density_kg_m3;
    double h2o_mass_fraction, liquid_loading_kg_kg, droplet_number_per_kg;
    double thermal_conductivity_w_m_k, dynamic_viscosity_pa_s;
    double cp_j_kg_k, gas_constant_j_kg_k, time_step_s;
    double nucleation_correction, maximum_nucleation_rate_m3_s;
    double maximum_vapor_consumption_fraction;
} Co2H2oCondensationInput;

typedef struct {
    double supersaturation, dewpoint_k, supercooling_k;
    double nucleation_rate_m3_s, nucleation_rate_raw_m3_s;
    double critical_radius_m, droplet_radius_m, growth_rate_m_s;
    double mass_source_raw_kg_m3_s, mass_source_limited_kg_m3_s;
    double liquid_density_kg_m3, surface_tension_n_m;
    double limiter_flag, risk_flag;
    double condensation_applied_kg_m3_s, evaporation_applied_kg_m3_s;
    double number_death_m3_s, number_source_m3_s;
    double birth_mass_applied_kg_m3_s, growth_mass_applied_kg_m3_s;
    double rate_cap_flag;
} Co2H2oCondensationOutput;

double co2h2o_psat_pa(double temperature_k);
double co2h2o_liquid_density_kg_m3(double temperature_k);
double co2h2o_surface_tension_n_m(double temperature_k);
double co2h2o_latent_heat_j_kg(double temperature_k);
double co2h2o_vapor_enthalpy_j_kg(double temperature_k);
double co2h2o_liquid_enthalpy_j_kg(double temperature_k);
double co2h2o_h2o_mass_to_mole_fraction(double h2o_mass_fraction);
double co2h2o_energy_source_w_m3(double gamma, double temperature_k,
                               double velocity_squared_m2_s2);
double co2h2o_energy_temperature_jacobian(
    const Co2H2oCondensationInput *input,
    double velocity_squared_m2_s2, double delta_temperature_k);
int co2h2o_evaluate(const Co2H2oCondensationInput *input,
                   Co2H2oCondensationOutput *output);
#endif
