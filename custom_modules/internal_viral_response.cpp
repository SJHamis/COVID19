#include "./internal_viral_response.h" 

using namespace PhysiCell; 

std::string internal_virus_response_version = "0.2.0"; 

Submodel_Information internal_virus_response_model_info; 

void internal_virus_response_model_setup( void )
{
	// set up the model 
		// set version info 
	internal_virus_response_model_info.name = "internal viral response"; 
	internal_virus_response_model_info.version = internal_virus_response_version; 
		// set functions 
	internal_virus_response_model_info.main_function = NULL; 
	internal_virus_response_model_info.phenotype_function = internal_virus_response_model; 
	internal_virus_response_model_info.mechanics_function = NULL; 
	
		// what microenvironment variables do you expect? 
		// what custom data do I need? 
	internal_virus_response_model_info.cell_variables.push_back( "max_infected_apoptosis_rate" ); 
	internal_virus_response_model_info.cell_variables.push_back( "max_apoptosis_half_max" ); 
	internal_virus_response_model_info.cell_variables.push_back( "apoptosis_hill_power" ); 	
	
		// register the submodel  
	internal_virus_response_model_info.register_model();	
		// set functions for the corresponding cell definition 
		
//	pCD = find_cell_definition( "lung epithelium" ); 
//	pCD->functions.update_phenotype = epithelium_submodel_info.phenotype_function;
//	pCD->functions.custom_cell_rule = epithelium_submodel_info.mechanics_function;
	
	return; 
}

void internal_virus_response_model( Cell* pCell, Phenotype& phenotype, double dt )
{
	static Cell_Definition* pCD = find_cell_definition( "lung epithelium" ); 
	
	// bookkeeping -- find microenvironment variables we need

	static int nV_external = microenvironment.find_density_index( "virion" ); 
	static int nA_external = microenvironment.find_density_index( "assembled virion" ); 
	
	static int nV_internal = pCell->custom_data.find_variable_index( "virion" ); 
	static int nA_internal = pCell->custom_data.find_variable_index( "assembled_virion" ); 
	
	// actual model goes here 

	// now, set apoptosis rate 
	
	static int apoptosis_model_index = cell_defaults.phenotype.death.find_death_model_index( "apoptosis" );
	// Sara&Fiona: We commented-out virus-induced apoptosis for now. 
	// To do: We will investigate pyroptosis VS apoptosis.  

	/** phenotype.death.rates[apoptosis_model_index] = base death rate (from cell line)
	double base_death_rate = 
		pCD->phenotype.death.rates[apoptosis_model_index]; 
	
	// additional death rate from infectoin  
	double additional_death_rate = pCell->custom_data["max_infected_apoptosis_rate"] ; 
	
	
	double v = pCell->custom_data[nA_internal] / 
		pCell->custom_data["max_apoptosis_half_max"] ; 
	v = pow( v, pCell->custom_data["apoptosis_hill_power"] ); 
	
	double effect = v / (1.0+v); 
	additional_death_rate *= effect; 
	phenotype.death.rates[apoptosis_model_index] = base_death_rate + additional_death_rate; 
	*/ 

	// if we're infected, secrete a chemokine for the immune model
	static int nAV = pCell->custom_data.find_variable_index( "assembled_virion" ); 	
	double AV = pCell->custom_data[nAV]; 
	
	static int nP = pCell->custom_data.find_variable_index( "viral_protein"); 
	double P = pCell->custom_data[nP];

	static int chemokine_index = microenvironment.find_density_index( "chemokine" ); 
	
/* old 
	if( P > 0.001 )
	{
		phenotype.secretion.secretion_rates[chemokine_index] = 
			pCell->custom_data[ "infected_cell_chemokine_secretion_rate" ];
		phenotype.secretion.saturation_densities[chemokine_index] = 1.0; 		
	}
	else
	{
		phenotype.secretion.secretion_rates[chemokine_index] = 0.0;
	}
*/	
	
	// if I am dead, make sure to still secrete the chemokine 
	
	// static int chemokine_index = microenvironment.find_density_index( "chemokine" ); 
	// static int nP = pCell->custom_data.find_variable_index( "viral_protein"); 
	// double P = pCell->custom_data[nP];
	
	// static int nAV = pCell->custom_data.find_variable_index( "assembled_virion" ); 
	// double AV = pCell->custom_data[nAV]; 
	static int proinflammatory_cytokine_index = microenvironment.find_density_index("pro-inflammatory cytokine");
		
	static int nR = pCell->custom_data.find_variable_index("viral_RNA");
	double R = pCell->custom_data[nR];
	
	if( R >= 1.00 - 1e-16 ) 
	{
		pCell->custom_data["infected_cell_chemokine_secretion_activated"] = 1.0; 
	}

	if( pCell->custom_data["infected_cell_chemokine_secretion_activated"] > 0.1 && phenotype.death.dead == false )
	{
		double rate = AV; 
		rate /= pCell->custom_data["max_apoptosis_half_max"];
		if( rate > 1.0 )
		{ rate = 1.0; }
		rate *= pCell->custom_data[ "infected_cell_chemokine_secretion_rate" ];

		phenotype.secretion.secretion_rates[chemokine_index] = rate; 
		phenotype.secretion.saturation_densities[chemokine_index] = 1.0;

		// (Adrianne) adding pro-inflammatory cytokine secretion by infected cells
		pCell->phenotype.secretion.secretion_rates[proinflammatory_cytokine_index] = pCell->custom_data["activated_cytokine_secretion_rate"];
	}
	

	if( pCell->custom_data["cell_pyroptosis_flag"]==1 )
	{
		pyroptosis_cascade( pCell, phenotype, dt ); 
		return;
	};
	
	// Sara&Fiona: Internalise pro-pyroptotic cytokine from the microenvironment
	static int pro_pyroptotic_cytokine = microenvironment.find_density_index("pro-pyroptosis cytokine"); 
	double pyroptotic_cytokine_concentration = phenotype.molecular.internalized_total_substrates[pro_pyroptotic_cytokine]; 

	//printf("PCC: %lf\n",pyroptotic_cytokine_concentration);

    //phenotype.secretion.uptake_rates[propyroptotic_cytokine_index] = 1.; // What should the secretion intake depend on ?
    //double pyroptotic_cytokine_concentration = phenotype.molecular.internalized_total_substrates[propyroptotic_cytokine_index];
    //phenotype.secretion.uptake_rates[nV_external] = pCell->custom_data[nR_bind] * pCell->custom_data[nR_EU]; 

    // (Sara&Fiona) pyroptosis cascade in the cell is initiated if cell's viral_RNA is >1 (i.e. >=3). This is arbitraty to check things work.
	if( R> 3 && pCell->custom_data["cell_pyroptosis_flag"]==0 && pCell->custom_data["cell_virus_induced_apoptosis_flag"]==0)
	{
		// set the probability (in 0,100) that a cell with a death-sentence pyroptoses (not apoptoses)
		int cell_death_pyroptosis_probability = 0; 
		// randomise a number in 0,100 that determines the cell death mode (pyroptosis or apoptosis)
		int cell_death_dice =  rand() % 101;
		if(cell_death_dice < cell_death_pyroptosis_probability) 
		{
			pCell->custom_data["cell_pyroptosis_flag"]=1; //cell pyroptoses
		}
		else 
		{
			pCell->custom_data["cell_virus_induced_apoptosis_flag"]=1; //cell apoptoses
			phenotype.death.rates[apoptosis_model_index] = 1000; 
		}


		
		return;
	}
    // (Sara&Fiona)
    else if(pyroptotic_cytokine_concentration>100.0 && pCell->custom_data["cell_pyroptosis_flag"]==0) 
    {
		pCell->custom_data["cell_pyroptosis_flag"]=1; // Pyroptosis cascade is initiated
		pCell->custom_data["cell_bystander_pyroptosis_flag"]=1; // Pyroptosis cascade is initiated
		//printf("Pyro bystander effect!\n");
		return;
    }
	
	return; 
}

// Sara&Fiona: code for pyroptosis
void pyroptosis_cascade( Cell* pCell, Phenotype& phenotype, double dt )
{
	// Sara&Fiona: Pyroptosis code starts here.
	
	// Intracellular components
	double nfkb_n = pCell->custom_data.find_variable_index( "nuclear_NFkB" ); 
	double nlrp3_i = pCell->custom_data.find_variable_index( "inactive_NLRP3" ); 
	double nlrp3_a = pCell->custom_data.find_variable_index( "active_NLRP3" ); 
	double nlrp3_b = pCell->custom_data.find_variable_index( "bound_NLRP3" ); 
	double asc_b = pCell->custom_data.find_variable_index( "bound_ASC" );
	double caspase1_b = pCell->custom_data.find_variable_index( "bound_caspase1" );
	double gsdmd_c = pCell->custom_data.find_variable_index( "cleaved_gasderminD" );
	double il_1b_p = pCell->custom_data.find_variable_index( "pro_IL_1b" );
	double il_1b_c = pCell->custom_data.find_variable_index( "cytoplasmic_IL_1b" );
	double il_1b_e = pCell->custom_data.find_variable_index( "external_IL_1b" );
	double il_18_c = pCell->custom_data.find_variable_index( "cytoplasmic_IL_18" );
	double il_18_e = pCell->custom_data.find_variable_index( "external_IL_18" );
	double volume_c = pCell->custom_data.find_variable_index( "cytoplasmic_volume" );
	//Not needed components (can be implicitely found via conservation laws)
	//static double nfkb_c = pCell->custom_data.find_variable_index( "cytoplasmic_NFkB_fraction" ); 
	//static double asc_f = pCell->custom_data.find_variable_index( "free_ASC" );
	//static double caspase1_f = pCell->custom_data.find_variable_index( "free_caspase1" );
	//static double gsdmd_uc = pCell->custom_data.find_variable_index( "uncleaved_gasderminD" );
	//static double il_18_e = pCell->custom_data.find_variable_index( "external_IL_1b" );

//We can add these to the xml later..
/**
	// Rate constants
	double k_nfkb_ctn = pCell->custom_data.find_variable_index( "rate_NFkB_cytoplasm_to_nucleus" ); 
	double k_nfkb_ntc = pCell->custom_data.find_variable_index( "rate_NFkB_nucleus_to_cytoplasm" );
	double k_nlrp3_ita = pCell->custom_data.find_variable_index( "rate_NLRP3_incactive_to_active" );
	double k_nlrp3_atb = pCell->custom_data.find_variable_index( "rate_NLRP3_active_to_bound" );
	double k_asc_ftb = pCell->custom_data.find_variable_index( "rate_ASC_free_to_bound" );
	double k_c1_ftb = pCell->custom_data.find_variable_index( "rate_caspase1_free_to_bound" );
	double k_il1b_cte = pCell->custom_data.find_variable_index( "rate_Il1b_cytoplasmic_to_external" );
	double k_il18_cte = 1;//pCell->custom_data.find_variable_index( "rate_Il18_cytoplasmic_to_external" );
	double k_vol_c = 1;// pCell->custom_data.find_variable_index( "rate_pyroptosis_volume_increase" );
	// Decay constants
	double d_nlrp3 = pCell->custom_data.find_variable_index( "decay_NLRPR_inactive_and_active" );
	double d_il = pCell->custom_data.find_variable_index( "decay_IL1b" );
	// Hill function rates
	double a_nlrp3 = pCell->custom_data.find_variable_index( "rate_constant_NLRP3_production" );
	double a_il1b_p = pCell->custom_data.find_variable_index( "rate_constant_IL1b_production" );
	double a_gsdmd = pCell->custom_data.find_variable_index( "rate_constant_GSDMD_cleavage" );
	double a_il1b_c = pCell->custom_data.find_variable_index( "rate_constant_Il1b_cleavage" );
	double a_il18 = pCell->custom_data.find_variable_index( "rate_constant_Il18_cleavage" );
	double hm_nfkb = pCell->custom_data.find_variable_index( "halfmax_NFkB_transcription" );
	
	//std::cout<<pCell->custom_data.find_variable_index( "halfmax_caspase1_cleavage" )<<std::endl;
	//double hm_c1 = 1;//pCell->custom_data.find_variable_index( "halfmax_caspase1_cleavage" );
	double hm_c1 = pCell->custom_data.find_variable_index("halfmax_caspase1_cleavage");
	double hex_nfkb = pCell->custom_data.find_variable_index( "hillexponent_NFkB_transcription" );
	double hex_c1 = pCell->custom_data.find_variable_index( "hillexponent_caspase1_cleavage" );
	// Total concentrations (let's have them all as 1 now and compute fractions)
	//static double tot_nfkb = pCell->custom_data.find_variable_index( "total_NFkB_concentration" );	
	//static double tot_asc = pCell->custom_data.find_variable_index( "total_ASC_concentration" );
	//static double tot_c1 = pCell->custom_data.find_variable_index( "total_caspase1_concentration" );
	//static double tot_gsdmd = pCell->custom_data.find_variable_index( "total_GSDMD_concentration" );
	//static double tot_il18 = pCell->custom_data.find_variable_index( "total_IL18_concentration" );

*/


	// System of pyroptosis equations starts here
    //Model constants (definitions could be moved to xml file)
    double k_nfkb_ctn = 0.3;
    double k_nfkb_ntc =  0.03;
    double k_nlrp3_ita =  0.07;
    double k_nlrp3_atb = 0.07;
    double k_asc_ftb = 0.02;
    double k_c1_ftb = 0.04;
    double k_il1b_cte = 0.8;
    double k_il18_cte = 0.8;
    double k_vol_c = 0.1;
    // Decay constants
    double d_nlrp3 = 0.002;
    double d_il =0.004;
    // Hill function rates
    double a_nlrp3 = 0.025;
    double a_il1b_p = 0.007;
    double a_gsdmd =0.08;
    double a_il1b_c = 0.8;
    double a_il18 = 0.8;
    double hm_nfkb = 0.3;
    double hm_c1 = 0.3;
    double hex_nfkb = 2.0;
    double hex_c1 = 2.0 ;
	//If the inflammsome base is formed set F_ib = 0. 
	double F_ib = 1;
	if( pCell->custom_data[nlrp3_b] >= 1)
	{F_ib=0;}

	//Update nuclear NFkB (updated backward)
	pCell->custom_data[nfkb_n] = (pCell->custom_data[nfkb_n]+k_nfkb_ctn*F_ib*dt)/(1+dt*k_nfkb_ntc+k_nfkb_ctn*F_ib*dt);

	//Set Hill function 1
	double hill_nfkb = (pow(pCell->custom_data[nfkb_n],hex_nfkb))/(pow(hm_nfkb,hex_nfkb)+pow(pCell->custom_data[nfkb_n],hex_nfkb));
	
	//Update NLRP3 (inactive, active and bound) (updated backward)
 	pCell->custom_data[nlrp3_i] = (pCell->custom_data[nlrp3_i]+dt*a_nlrp3*hill_nfkb)/(1+dt*k_nlrp3_ita+dt*d_nlrp3);
	pCell->custom_data[nlrp3_a] = (pCell->custom_data[nlrp3_a]+k_nlrp3_ita*dt*(pCell->custom_data[nlrp3_i]))/(1+dt*k_nlrp3_atb+dt*d_nlrp3);
	pCell->custom_data[nlrp3_b] = pCell->custom_data[nlrp3_b] + dt * k_nlrp3_atb * F_ib * pCell->custom_data[nlrp3_a];

	//Update bound ASC (updated backward)
	pCell->custom_data[asc_b] = (pCell->custom_data[asc_b] + dt*k_asc_ftb*(1-F_ib)*(pCell->custom_data[nlrp3_b]))/(1+dt*k_asc_ftb*(1-F_ib)*(pCell->custom_data[nlrp3_b]));

	//Update bound caspase1 (updated backward)
	pCell->custom_data[caspase1_b] = (pCell->custom_data[caspase1_b] + dt*k_c1_ftb*(pCell->custom_data[asc_b]))/(1+dt*k_c1_ftb*(pCell->custom_data[asc_b])); 

	//Set Hill function 2
	double hill_caspase1 = (pow(pCell->custom_data[caspase1_b],hex_c1))/(pow(hm_c1,hex_c1)+pow(pCell->custom_data[caspase1_b],hex_c1));
	
	//Update cleaved GSDMD (updated backward)
	pCell->custom_data[gsdmd_c] = (pCell->custom_data[gsdmd_c]+dt*a_gsdmd*hill_caspase1)/(1+dt*a_gsdmd*hill_caspase1);

	//Set G function (same now that total GSDMD concentration is 1 au of concentration)
	double g_gsdmd = pCell->custom_data[gsdmd_c]/1;

	//Update IL1b (pro, cytoplasmic, external)	We want to relate this to secreted cytokine IL1b (updated backward)
	pCell->custom_data[il_1b_p] = (pCell->custom_data[il_1b_p]+dt*a_il1b_p*hill_nfkb)/(1+dt*a_il1b_c*hill_caspase1+dt*d_il);
	pCell->custom_data[il_1b_c] = (pCell->custom_data[il_1b_c]+dt*a_il1b_c*hill_caspase1*(pCell->custom_data[il_1b_p]))/(1+dt*d_il+dt*k_il1b_cte*g_gsdmd);		
	pCell->custom_data[il_1b_e] = pCell->custom_data[il_1b_e] + dt * (k_il1b_cte*g_gsdmd*pCell->custom_data[il_1b_c]);

	//Update IL18 (cytoplasmic, external)(updated backward)
	pCell->custom_data[il_18_c] = (pCell->custom_data[il_18_c]+dt*a_il18*hill_caspase1*(1-pCell->custom_data[il_18_e]))/((1+dt*a_il18*hill_caspase1)*(1+dt*k_il18_cte*g_gsdmd));
	pCell->custom_data[il_18_e] = pCell->custom_data[il_18_e] +  dt * k_il18_cte*g_gsdmd*pCell->custom_data[il_18_c];

	//Update cytoplasmic volume (updated backward)
	pCell->custom_data[volume_c] = pCell->custom_data[volume_c]/(1-dt * k_vol_c * g_gsdmd);

	//Temporary: "super fast" apoptosis occurs when cell should burst. 
	//To do: We actually want the cell to rupture once a cytoplasmic critical volume is reached (e.g. 1.5 of initial cytoplasmic volume from in vitro data). 
	static int apoptosis_model_index = cell_defaults.phenotype.death.find_death_model_index( "apoptosis" );	
	if( pCell->custom_data[volume_c]>1.2)
	{
		//std::cout<<"Pyroptotic cell burst!"<<std::endl;
		//The cell's 'apoptosis death rate' is set to be "super high" 
		phenotype.death.rates[apoptosis_model_index] = 9e9; 
	}

	// (Adrianne) update cell pro-inflammatory secretion rate based on IL18 secretion rate - need to double check unit conversion
	static int proinflammatory_cytokine_index = microenvironment.find_density_index( "pro-inflammatory cytokine");
	pCell->phenotype.secretion.secretion_rates[proinflammatory_cytokine_index] = pCell->custom_data["activated_cytokine_secretion_rate"]+k_il18_cte*g_gsdmd*pCell->custom_data[il_18_c];
    // (Sara and Fiona)
	static int propyroptotic_cytokine_index = microenvironment.find_density_index("pro-pyroptosis cytokine");
	pCell->phenotype.secretion.secretion_rates[propyroptotic_cytokine_index] = k_il1b_cte*g_gsdmd*pCell->custom_data[il_1b_c];

	//To do: We also want IL-1beta (pCell->custom_data[il_1b_e]) to be secreted. 
	//IL-1beta can induce pyroptosis in other cells (pyroptosis bystander effect). 
	
	return;
}
