"""

Purpose:
     Create a class whose attributes have all the necessary inputs for
     performing a Fresnel inversion usng DiffractionCorrection, given
     instances of the classes RSRReader, Geometry, and Calibration.

"""

import copy
import numpy as np
from scipy.interpolate import splrep, splev, interp1d

from ..rsr_reader.rsr_reader import RSRReader
from .resample_IQ import resample_IQ
from .calc_tau_thresh import calc_tau_thresh
from ..tools.history import write_history_dict
from ..tools.write_output_files import write_output_files

class DiffractionLimitedProfile(object):
    """
    Framework for an object class containing the diffraction-limited
    optical depth profile (DLP) and related attributes.

    Arguments:
        :rsr_inst (*object*): Instance of RSRReader class
        :geo_inst (*object*): Instance of Geometry class
        :cal_inst (*object*): Instance of Calibration class
        :dr_km (*float*): radial sampling rate :math:`\\Delta\\rho`
                        for the DLP in kilometers. DLP radial
                        *resolution* is the Nyquist radial sampling,
                        i.e., twice the input value of `dr_km`,
                        meaning that this will affect the minimum
                        resolution of the diffraction-reconstructed
                        profile. Value for `dr_km` can range from
                        0.05 km to 0.75 km for the reconstruction
                        resolutions supported by `rss_ringoccs`.
                        PDS sampling rate is 0.25 km, which gives a
                        DLP resolution of 0.5 km.

    Keyword Arguments:
        :verbose (*bool*): When True, turns on verbose output. Default
                        is False.
        :write_file (*bool*): When True, writes processing results to
                        file. Default is True.
        :profile_range (*list*): 1x2 list specifying the radial limits
                        in km of on the occultation. Default is
                        [65000,150000].

    Attributes:
        :dr_km (*float*): raw DLP sampling rate
        :tau_threshold_vals (*np.ndarray*): threshold optical depth

        :rho_km_vals (*np.ndarray*): Ring-intercept points in km
        :t_oet_spm_vals (*np.ndarray*): Observed event times in seconds past
                                        midnight
        :p_norm_vals (*np.ndarray*): Normalized diffraction-limited power
        :phase_rad_vals (*np.ndarray*): Phase of diffraction-limited signal,
                                        in radians
        :B_rad_vals (*np.ndarray*): Ring opening angle in radians
        :D_km_vals (*np.ndarray*): Ring intercept point to spacecraft distance
                                   in km
        :F_km_vals (*np.ndarray*): Fresnel scale in km
        :f_sky_hz_vals (*np.ndarray*): Sky frequency in Hz
        :phi_rad_vals (*np.ndarray*): Observed ring azimuth
        :t_ret_spm_vals (*np.ndarray*): Ring event time in seconds past midnight
        :t_set_spm_vals (*np.ndarray*): Spacecraft event time in seconds past
                                        midnight
        :phi_rl_rad_vals (*np.ndarray*): Ring longitude in radians
        :rho_dot_kms_vals (*np.ndarray*): Ring intercept radial velocity in km/s
        :rho_corr_pole_km_vals (*np.ndarray*): Radius correction due to 
                                               improved pole. This is populated
                                               with a placeholder of zeros
        :rho_corr_timing_km_vals (*np.ndarray*): Radius correction due to 
                                                 timing offset. This is
                                                 populated with a placeholder 
                                                 of zeros
        :tau_vals (*np.ndarray*): Diffraction-limited optical depth
        :history (*dict*): Processing history with all inputs necessary to
                           rerun pipeline to obtain identical output

    Note:
        #. All *np.ndarray* attributes are sampled at dr_km radial spacing.
    """
    def __init__(self, rsr_inst, geo_inst, cal_inst, dr_km, verbose=False, 
            write_file=True, profile_range=[65000., 150000.]):


        if verbose:
            print('Preparing diffraction-limited profile...')

        # Extract necessary information from instance inputs
        spm_full = rsr_inst.spm_vals
        IQ_m = rsr_inst.IQ_m

        spm_geo = geo_inst.t_oet_spm_vals
        rho_km_geo = geo_inst.rho_km_vals
        rho_dot_kms_geo = geo_inst.rho_dot_kms_vals
        prof_dir = geo_inst.rev_info['prof_dir']
        B_deg_geo = geo_inst.B_deg_vals
        F_km_geo = geo_inst.F_km_vals
        t_ret_spm_geo = geo_inst.t_ret_spm_vals
        t_set_spm_geo = geo_inst.t_set_spm_vals
        phi_ora_deg_geo = geo_inst.phi_ora_deg_vals
        phi_rl_deg_geo = geo_inst.phi_rl_deg_vals
        D_km_geo = geo_inst.D_km_vals
        prof_dir = geo_inst.rev_info['prof_dir']

        spm_cal = cal_inst.t_oet_spm_vals
        f_sky_pred_cal = cal_inst.f_sky_hz_vals
        p_free_cal = cal_inst.p_free_vals
        IQ_c = cal_inst.IQ_c

        # interpolate coef to convert SPM to rho
        spm_to_rho = splrep(spm_geo, rho_km_geo)
        rho_full = splev(spm_full, spm_to_rho)

        rho_km_cal = splev(spm_cal, spm_to_rho)

        # resample IQ_c, but not to exact rho boundaries
        rho_km_desired, IQ_c_desired = resample_IQ(rho_full, IQ_c,
                dr_km)

        # Slice rho_km_desired to profile_range values
        ind1 = list(rho_km_desired).index(min(list(rho_km_desired),
                    key=lambda x:abs(x-profile_range[0])))

        ind2 = list(rho_km_desired).index(min(list(rho_km_desired),
                    key=lambda x:abs(x-profile_range[1])))
        rho_km_desired = rho_km_desired[ind1:ind2+1]
        IQ_c_desired = IQ_c_desired[ind1:ind2+1]

        # interpolate coef to convert rho to SPm
        spm_desired_func = interp1d(rho_km_geo, spm_geo,
                fill_value='extrapolate')
        spm_desired = spm_desired_func(rho_km_desired)

        p_free_interp_func = interp1d(rho_km_cal, p_free_cal,
                fill_value='extrapolate')
        p_free = p_free_interp_func(rho_km_desired)

        p_norm_vals = (abs(IQ_c_desired)**2)/p_free

        # Check for negative power
        if min(p_norm_vals) < 0.:
            raise ValueError('(dlp_class.py): Negative power values found '
                    + 'within ring system!')

        phase_rad_vals = np.arctan2(np.imag(IQ_c_desired),
                np.real(IQ_c_desired))

        # compute threshold optical depth at Nyquist sampling rate
        # (i.e., twice the "raw" DLP sampling rate)
        tau_thresh_inst = calc_tau_thresh(rsr_inst,geo_inst,cal_inst,
                res_km=dr_km)
        tau_thresh = tau_thresh_inst.tau_thresh
        spm_thresh = tau_thresh_inst.spm_vals

        self.__interp_and_set_attr(rho_km_desired, spm_desired, p_norm_vals,
                spm_cal, phase_rad_vals, spm_geo, rho_dot_kms_geo,
                B_deg_geo, F_km_geo, t_ret_spm_geo, t_set_spm_geo,
                D_km_geo, phi_ora_deg_geo, phi_rl_deg_geo, f_sky_pred_cal,
                tau_thresh, spm_thresh, prof_dir)


        # if set, write output data and label file
        self.rev_info = geo_inst.rev_info
        self.dr_km = dr_km


        input_vars = {
                'rsr_inst': rsr_inst.history,
                'geo_inst': geo_inst.history,
                'cal_inst': cal_inst.history,
                'dr_km': dr_km}

        input_kwds = {
                'profile_range': profile_range}

        self.history = write_history_dict(input_vars, input_kwds, __file__)
        if write_file:
            write_output_files(self)



    def __interp_and_set_attr(self, rho_km_desired, spm_desired,
            p_norm_vals, spm_cal, phase_rad_vals, spm_geo, rho_dot_kms_geo,
            B_deg_vals, F_km_geo, t_ret_geo, t_set_geo,
            D_km_geo, phi_ora_deg_vals, phi_rl_deg_vals, f_sky_pred_cal,
            tau_thresh, spm_thresh, prof_dir):

        B_rad_geo = np.radians(B_deg_vals)
        phi_ora_rad_geo = np.radians(phi_ora_deg_vals)
        phi_rl_rad_geo = np.radians(phi_rl_deg_vals)

        # ring longitude at final spacing
        spm_to_rhodot = splrep(spm_geo, rho_dot_kms_geo)
        rho_dot_kms_vals_interp = splev(spm_desired, spm_to_rhodot)

        # check if rho_dot is both positive and negative,
        #   if so, remove unwanted values
        if min(rho_dot_kms_vals_interp)<0 and max(rho_dot_kms_vals_interp)>0:
            if prof_dir == '"INGRESS"':
                ind = np.argwhere(rho_dot_kms_vals_interp > 0)
            if prof_dir == '"EGRESS"':
                ind = np.argwhere(rho_dot_kms_vals_interp < 0)
            spm_desired = np.delete(spm_desired, ind)
            rho_km_desired = np.delete(rho_km_desired, ind)
            p_norm_vals = np.delete(p_norm_vals, ind)
            phase_rad_vals = np.delete(phase_rad_vals, ind)
            rho_dot_kms_vals_interp = splev(spm_desired, spm_to_rhodot)

        # ring opening angle at final spacing
        spm_to_B = splrep(spm_geo, B_rad_geo)
        B_rad_vals_interp = splev(spm_desired, spm_to_B)

        # spacecraft - rip distance at final spacing
        spm_to_D = splrep(spm_geo, D_km_geo)
        D_km_vals_interp = splev(spm_desired, spm_to_D)

        # Fresnel scale at final spacing
        spm_to_F = splrep(spm_geo, F_km_geo)
        F_km_vals_interp = splev(spm_desired, spm_to_F)

        # sky frequency at final spacing
        spm_to_fsky = splrep(spm_cal, f_sky_pred_cal)
        f_sky_hz_vals_interp = splev(spm_desired, spm_to_fsky)

        # obvserved ring azimuth at final spacing
        spm_to_phi_ora = splrep(spm_geo, phi_ora_rad_geo)
        phi_ora_rad_vals_interp = splev(spm_desired, spm_to_phi_ora)

        # obvserved ring azimuth at final spacing
        spm_to_phi_rl = splrep(spm_geo, phi_rl_rad_geo)
        phi_rl_rad_vals_interp = splev(spm_desired, spm_to_phi_rl)

        # ring event time at final spacing
        spm_to_ret = splrep(spm_geo, t_ret_geo)
        t_ret_spm_vals_interp = splev(spm_desired, spm_to_ret)

        # spacecraft event time at final spacing
        spm_to_set = splrep(spm_geo, t_set_geo)
        t_set_spm_vals_interp = splev(spm_desired, spm_to_set)


        # FILLERS FOR RADIUS CORRECTION
        rho_corr_pole_km_vals = np.zeros(len(spm_desired))
        rho_corr_timing_km_vals = np.zeros(len(spm_desired))

        # Set attributes
        self.rho_km_vals = rho_km_desired
        self.t_oet_spm_vals = spm_desired
        self.p_norm_vals = p_norm_vals
        self.phase_rad_vals = phase_rad_vals

        self.B_rad_vals = B_rad_vals_interp
        self.D_km_vals = D_km_vals_interp
        self.F_km_vals = F_km_vals_interp
        self.f_sky_hz_vals = f_sky_hz_vals_interp
        self.phi_rad_vals = phi_ora_rad_vals_interp
        self.t_ret_spm_vals = t_ret_spm_vals_interp
        self.t_set_spm_vals = t_set_spm_vals_interp
        self.phi_rl_rad_vals = phi_rl_rad_vals_interp
        self.rho_dot_kms_vals = rho_dot_kms_vals_interp
        self.rho_corr_pole_km_vals = rho_corr_pole_km_vals
        self.rho_corr_timing_km_vals = rho_corr_timing_km_vals
        self.tau_vals = -np.sin(B_rad_vals_interp)*np.log(p_norm_vals)
        self.tau_threshold_vals = np.interp(
                spm_desired, spm_thresh, tau_thresh)


    @classmethod
    def create_dlps(cls, rsr_inst, geo_inst, cal_inst, dr_km, 
        verbose=False, write_file=False, profile_range=[65000., 150000.]):
        """
        Purpose:
            Create ingress and egress instances of DiffractionLimitedProfile.

        Arguments:
            :rsr_inst (*object*): Instance of RSRReader class
            :geo_inst (*object*): Instance of Geometry class
            :cal_inst (*object*): Instance of Calibration class
            :dr_km (*float*): radial sampling rate :math:`\\Delta\\rho`
                            for the DLP in kilometers. DLP radial
                            *resolution* is the Nyquist radial sampling,
                            i.e., twice the input value of `dr_km`,
                            meaning that this will affect the minimum
                            resolution of the diffraction-reconstructed
                            profile. Value for `dr_km` can range from
                            0.05 km to 0.75 km for the reconstruction
                            resolutions supported by `rss_ringoccs`.
                            PDS sampling rate is 0.25 km, which gives a
                            DLP resolution of 0.5 km.

        Keyword Arguments:
            :verbose (*bool*): When True, turns on verbose output. Default
                            is False.
            :write_file (*bool*): When True, writes processing results to
                            file. Default is True.
            :profile_range (*list*): 1x2 list specifying the radial limits
                            in km of on the occultation. Default is
                            [65000,150000].
        """
        # check for ingress, egress, or chord
        prof_dir = geo_inst.rev_info['prof_dir']

        spm_full = rsr_inst.spm_vals
        spm_geo = geo_inst.t_oet_spm_vals

        # Check if spm_geo and spm_full start/end at same time,
        #   if not, reduce spm_full to spm_geo boundaries
        if (min(spm_geo) != np.floor(min(spm_full))) or (
                max(spm_geo) != np.floor(max(spm_full))):
            ind1 = np.where(spm_full == spm_geo[0])[0][0]
            ind2 = np.where(spm_full == spm_geo[-1])[0][0]

            IQ_m = rsr_inst.IQ_m
            IQ_c = cal_inst.IQ_c

            rsr_inst.spm_vals = spm_full[ind1:ind2]
            rsr_inst.IQ_m = IQ_m[ind1:ind2]
            cal_inst.IQ_c = IQ_c[ind1:ind2]


        if prof_dir == '"INGRESS"':
            dlp_egr = None
            dlp_ing = cls(rsr_inst, geo_inst, cal_inst, dr_km, verbose=verbose,
                    write_file=write_file, profile_range=profile_range)

        elif prof_dir == '"EGRESS"':
            dlp_ing = None
            dlp_egr = cls(rsr_inst, geo_inst, cal_inst, dr_km, verbose=verbose,
                    write_file=write_file, profile_range=profile_range)

        elif prof_dir == '"BOTH"':
            # split ingress and egress
            geo_ing = copy.deepcopy(geo_inst)
            geo_egr = copy.deepcopy(geo_inst)

            # Get index of first value after rho_dot switches value
            ind = np.argwhere(
                    np.diff(np.sign(geo_inst.rho_dot_kms_vals)))[0][0]+1
            ind_spm1 = geo_inst.t_oet_spm_vals[ind]

            rsr_ing = copy.deepcopy(rsr_inst)
            rsr_egr = copy.deepcopy(rsr_inst)

            cal_ing = copy.deepcopy(cal_inst)
            cal_egr = copy.deepcopy(cal_inst)

            # For ingress occ, remove 1s from the end
            ind_rsr_ing = np.argwhere(
                            rsr_inst.spm_vals == (ind_spm1-1.))[0][0]
            ind_cal_ing = np.argwhere(
                            cal_inst.t_oet_spm_vals == (ind_spm1-1.))[0][0]

            # For egress occ, remove 1s from the beginning
            ind_rsr_egr = np.argwhere(
                            rsr_inst.spm_vals == (ind_spm1+1.))[0][0]
            ind_cal_egr = np.argwhere(
                            cal_inst.t_oet_spm_vals == (ind_spm1+1.))[0][0]

            # split raw spm and IQ attributes
            rsr_ing.spm_vals = rsr_inst.spm_vals[:ind_rsr_ing]
            rsr_ing.IQ_m = rsr_inst.IQ_m[:ind_rsr_ing]

            rsr_egr.spm_vals = rsr_inst.spm_vals[ind_rsr_egr:]
            rsr_egr.IQ_m = rsr_inst.IQ_m[ind_rsr_egr:]


            # split calibration attributes
            cal_ing.IQ_c = cal_inst.IQ_c[:ind_rsr_ing]

            cal_ing.p_free_vals = cal_inst.p_free_vals[:ind_cal_ing]
            cal_ing.f_sky_hz_vals = cal_inst.f_sky_hz_vals[:ind_cal_ing]
            cal_ing.t_oet_spm_vals = cal_inst.t_oet_spm_vals[:ind_cal_ing]
            cal_ing.f_sky_resid_fit_vals = (
                        cal_inst.f_sky_resid_fit_vals[:ind_cal_ing])

            cal_egr.IQ_c = cal_inst.IQ_c[ind_rsr_egr:]
            cal_egr.p_free_vals = cal_inst.p_free_vals[ind_cal_egr:]
            cal_egr.f_sky_hz_vals = cal_inst.f_sky_hz_vals[ind_cal_egr:]
            cal_egr.t_oet_spm_vals = cal_inst.t_oet_spm_vals[ind_cal_egr:]
            cal_egr.f_sky_resid_fit_vals = (
                        cal_inst.f_sky_resid_fit_vals[ind_cal_egr:])

            # split geometry attributes
            geo_ing.rho_km_vals = geo_inst.rho_km_vals[:ind]
            geo_ing.rho_dot_kms_vals = geo_inst.rho_dot_kms_vals[:ind]
            geo_ing.t_oet_spm_vals = geo_inst.t_oet_spm_vals[:ind]
            geo_ing.t_ret_spm_vals = geo_inst.t_ret_spm_vals[:ind]
            geo_ing.t_set_spm_vals = geo_inst.t_set_spm_vals[:ind]
            geo_ing.B_deg_vals = geo_inst.B_deg_vals[:ind]
            geo_ing.phi_ora_deg_vals = geo_inst.phi_ora_deg_vals[:ind]
            geo_ing.phi_rl_deg_vals = geo_inst.phi_rl_deg_vals[:ind]
            geo_ing.D_km_vals = geo_inst.D_km_vals[:ind]
            geo_ing.F_km_vals = geo_inst.F_km_vals[:ind]
            geo_ing.rev_info['prof_dir'] = '"INGRESS"'

            dlp_ing = cls(rsr_ing, geo_ing, cal_ing, dr_km, verbose=verbose,
                    write_file=write_file, profile_range=profile_range)


            geo_egr.rho_km_vals = geo_inst.rho_km_vals[ind:]
            geo_egr.rho_dot_kms_vals = geo_inst.rho_dot_kms_vals[ind:]
            geo_egr.t_oet_spm_vals = geo_inst.t_oet_spm_vals[ind:]
            geo_egr.t_ret_spm_vals = geo_inst.t_ret_spm_vals[ind:]
            geo_egr.t_set_spm_vals = geo_inst.t_set_spm_vals[ind:]
            geo_egr.B_deg_vals = geo_inst.B_deg_vals[ind:]
            geo_egr.phi_ora_deg_vals = geo_inst.phi_ora_deg_vals[ind:]
            geo_egr.phi_rl_deg_vals = geo_inst.phi_rl_deg_vals[ind:]
            geo_egr.D_km_vals = geo_inst.D_km_vals[ind:]
            geo_egr.F_km_vals = geo_inst.F_km_vals[ind:]
            geo_egr.rev_info['prof_dir'] = '"EGRESS"'
            dlp_egr = cls(rsr_egr, geo_egr, cal_egr, dr_km, verbose=verbose, 
                    write_file=write_file, profile_range=profile_range)
        else:
            raise ValueError('ERROR (create_dlps()): Invalid profile '
                + 'direction given!')

        return dlp_ing, dlp_egr

"""
Revisions:
"""
